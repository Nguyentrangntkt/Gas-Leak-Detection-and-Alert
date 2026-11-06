/**
 * @file    esp32_blynk.ino
 * @brief   Cau noi Wi-Fi: nhan telemetry tu F103 qua UART roi day len Blynk.
 */
#include "secrets.h"
#include "config.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

static char     g_line[32];
static uint8_t  g_len = 0;
static bool     g_overflow = false;

static uint32_t g_ppm = 0;
static char     g_code = 'W';
static bool     g_link_up = false;

static uint32_t g_last_frame_ms = 0;
static uint32_t g_last_alert_ms = 0;
static bool     g_alert_active  = false;
static uint8_t  g_alert_level   = 0;

static uint32_t g_last_fault_ms = 0;
static bool     g_fault_active  = false;

BlynkTimer timer;

static bool parseTelemetry(const char *line, uint8_t len,
                           uint32_t *ppm_out, char *code_out)
{
  if (len != 8 || line[0] != 'P' || line[6] != ',') {
    return false;
  }

  uint32_t v = 0;
  for (uint8_t i = 1; i <= 5; i++) {
    if (line[i] < '0' || line[i] > '9') {
      return false;
    }
    v = v * 10 + (uint32_t)(line[i] - '0');
  }

  const char c = line[7];
  if (c != '0' && c != '1' && c != '2' && c != '3' &&
      c != 'W' && c != 'F' && c != 'O') {
    return false;
  }

  *ppm_out  = v;
  *code_out = c;
  return true;
}

static void readSerial()
{
  while (Serial2.available() > 0) {
    const char c = (char)Serial2.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      if (!g_overflow && g_len > 0) {
        uint32_t ppm;
        char code;
        if (parseTelemetry(g_line, g_len, &ppm, &code)) {
          g_ppm  = ppm;
          g_code = code;
          g_last_frame_ms = millis();
          g_link_up = true;
        }
      }
      g_len = 0;
      g_overflow = false;
      continue;
    }

    if (g_len >= sizeof(g_line)) {
      g_overflow = true;
      continue;
    }
    g_line[g_len++] = c;
  }
}

static bool codeIsGasLevel(char code)
{
  return (code >= '0' && code <= '3');
}

static const char *codeToText(char code)
{
  switch (code) {
    case '0': return "Muc 0 - duoi nguong thiet ke";
    case '1': return "Muc 1 - canh bao";
    case '2': return "Muc 2 - nguy hiem";
    case '3': return "Muc 3 - nguy hiem cao";
    case 'W': return "Dang lam nong cam bien";
    case 'F': return "LOI CAM BIEN - khong do duoc";
    case 'O': return "He thong dang dung";
    default:  return "Ma trang thai la";
  }
}

static void reportFault()
{
  const bool cooled = (!g_fault_active) ||
                      ((uint32_t)(millis() - g_last_fault_ms) >=
                       CFG_FAULT_COOLDOWN_MS);
  if (cooled) {
    Blynk.logEvent(CFG_BLYNK_EVENT_FAULT,
                   String("Loi cam bien MQ-2: thiet bi khong do duoc nong "
                          "do. Day KHONG phai trang thai an toan."));
    g_last_fault_ms = millis();
    g_fault_active  = true;
  }
}

static void reportGas(uint8_t level)
{

  const bool cooled = (!g_alert_active) ||
                      ((uint32_t)(millis() - g_last_alert_ms) >=
                       CFG_ALERT_COOLDOWN_MS);
  if (cooled || level > g_alert_level) {
    Blynk.logEvent(CFG_BLYNK_EVENT,
                   String("Phat hien ro ri khi gas: ") + g_ppm +
                   " ppm (muc " + level + ")");
    g_last_alert_ms = millis();
    g_alert_active  = true;
    g_alert_level   = level;
  }
}

static void pushToBlynk()
{

  if ((uint32_t)(millis() - g_last_frame_ms) >= CFG_LINK_TIMEOUT_MS) {
    g_link_up = false;
  }

  if (!Blynk.connected()) {
    return;
  }

  Blynk.virtualWrite(CFG_VPIN_ONLINE, g_link_up ? 1 : 0);

  if (!g_link_up) {
    Blynk.virtualWrite(CFG_VPIN_STATUS, "Mat ket noi voi bo do");
    return;
  }

  Blynk.virtualWrite(CFG_VPIN_STATUS, codeToText(g_code));

  if (!codeIsGasLevel(g_code)) {

    if (g_code == 'F') {
      reportFault();
    }
    g_alert_active = false;
    g_alert_level  = 0;
    return;
  }

  g_fault_active = false;

  const uint8_t level = (uint8_t)(g_code - '0');
  Blynk.virtualWrite(CFG_VPIN_PPM, g_ppm);
  Blynk.virtualWrite(CFG_VPIN_LEVEL, level);

  if (level >= CFG_ALERT_FROM_LEVEL) {
    reportGas(level);
  } else {

    g_alert_active = false;
    g_alert_level  = 0;
  }
}

static uint32_t g_net_next_try_ms = 0;
static uint32_t g_net_backoff_ms  = CFG_NET_RETRY_MIN_MS;

static void netBackoffAdvance()
{
  g_net_next_try_ms = millis() + g_net_backoff_ms;
  g_net_backoff_ms *= 2UL;
  if (g_net_backoff_ms > CFG_NET_RETRY_MAX_MS) {
    g_net_backoff_ms = CFG_NET_RETRY_MAX_MS;
  }
}

static bool netDue()
{
  return ((int32_t)(millis() - g_net_next_try_ms) >= 0);
}

static void serviceNetwork()
{
  if (WiFi.status() != WL_CONNECTED) {
    if (netDue()) {
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      netBackoffAdvance();
    }
    return;
  }

  if (!Blynk.connected()) {
    if (netDue()) {
      netBackoffAdvance();
      (void)Blynk.connect(CFG_BLYNK_CONNECT_MS);
    }
    return;
  }

  g_net_backoff_ms = CFG_NET_RETRY_MIN_MS;
  Blynk.run();
}

void setup()
{
  Serial.begin(115200);

  Serial2.setRxBufferSize(CFG_UART_RX_BUFFER);
  Serial2.begin(CFG_UART_BAUD, SERIAL_8N1, CFG_UART_RX_PIN, CFG_UART_TX_PIN);

  Serial.println();
  Serial.println(F("Gas leak detector - ESP32 Blynk bridge"));

  WiFi.mode(WIFI_STA);
  Blynk.config(BLYNK_AUTH_TOKEN);

  g_last_frame_ms   = millis();
  g_net_next_try_ms = millis();
  timer.setInterval(CFG_PUSH_PERIOD_MS, pushToBlynk);
}

void loop()
{

  readSerial();

  serviceNetwork();
  timer.run();
}

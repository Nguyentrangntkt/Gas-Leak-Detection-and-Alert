/**
 * @file    secrets_example.h
 * @brief   Mau cau hinh bi mat. CHEP file nay thanh secrets.h roi dien gia tri.
 *
 *   cp secrets_example.h secrets.h
 *
 * secrets.h nam trong .gitignore va KHONG duoc commit.
 */
#ifndef SECRETS_H
#define SECRETS_H

/* Lay tu Blynk Console > Developer Zone > My Templates */
#define BLYNK_TEMPLATE_ID    "TMPLxxxxxxxx"
#define BLYNK_TEMPLATE_NAME  "Gas Leak Detector"
#define BLYNK_AUTH_TOKEN     "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

/* Wi-Fi 2.4 GHz (ESP32 khong ket noi duoc bang 5 GHz) */
#define WIFI_SSID            "ten-wifi"
#define WIFI_PASSWORD        "mat-khau-wifi"

#endif /* SECRETS_H */

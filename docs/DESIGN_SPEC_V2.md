# DESIGN SPECIFICATION V2 — ĐÃ DUYỆT

Hệ thống phát hiện và cảnh báo rò rỉ khí LPG — bản chuẩn hóa thiết kế
Nguồn gốc: Report V1.1 (51 trang) + 2 project STM32CubeIDE Empty Project
Trạng thái: **đã duyệt và đã khôi phục firmware theo tài liệu này**

---

## 0. Phân loại bằng chứng

Mọi khẳng định trong tài liệu này đều mang một trong năm nhãn sau. Không gộp chúng với nhau.

| Nhãn | Nghĩa |
|---|---|
| **VERIFIED HARDWARE** | Thông tin phần cứng thực tế nhóm đã xác nhận |
| **DATASHEET** | Thông số công bố trong tài liệu kỹ thuật của linh kiện/MCU |
| **CALCULATED** | Giá trị suy ra bằng phép tính từ thông số và giả thiết đã nêu rõ |
| **DESIGN DECISION** | Phương án kỹ thuật được chọn cho bản khôi phục |
| **UNVERIFIED** | Chưa có dữ liệu đo hoặc kiểm thử thực tế |

Phạm vi đã chốt: khôi phục firmware từ báo cáo, sửa lỗi kỹ thuật đã xác minh, chuẩn hóa cấu trúc, biên dịch được, kiểm thử bằng dữ liệu mô phỏng. **Không thay đổi phần cứng, không thêm linh kiện, không mở rộng tính năng.**

---

## 1. Kiến trúc

```
            ┌──────────────┐
   MQ-2 ───▶│              │  I2C1 (PB6/PB7)
  AOUT 3.3V │ STM32F103C8T6│──────────────▶ LCD 16x2 I2C @0x27
   → PA0    │   PRODUCER   │
   (ADC1)   │              │  USART1 (PA9 TX / PA10 RX)   ┌──────────────┐
            │              │◀────────────────────────────▶│STM32F401CCU6 │
            │              │   "S<code>\n"  →             │   CONSUMER   │
            │              │   ← "I\n" / "O\n"            │   USART2     │
            │              │                              │  (PA2/PA3)   │
            │              │  USART2 (PA2 TX, một chiều)  │              │
            └──────┬───────┘                              └──────┬───────┘
                   │  "P<ppm>,<code>\n"                          │
                   ▼                                             ├─ PB0 LED xanh dương
            ┌──────────────┐                                     ├─ PB1 LED vàng
            │    ESP32     │── Wi-Fi ──▶ Blynk Cloud             ├─ PB2 LED đỏ
            │  GPIO16 RX   │  V0 ppm · V1 mức · V2 online        ├─ PB3 LED xanh lá
            └──────────────┘  logEvent "gas_alert"               ├─ PA1 Buzzer
                                                                 ├─ PB5 Tín hiệu relay
                                                                 └─ PC13 Start/Stop (EXTI13)
```

**Phân chia trách nhiệm:** F103 là nơi duy nhất biết ppm; F401 là nơi duy nhất biết công tắc và điều khiển cơ cấu chấp hành. F401 không có đường nào tự suy ra ppm — đó là chủ ý, giúp mỗi MCU kiểm thử độc lập được.

Luồng chi tiết và quy tắc phân tích khung: xem `PROTOCOL.md`.

---

## 2. Pin mapping

Xem `PIN_MAPPING.md` (bảng đầy đủ kèm nhãn bằng chứng từng dòng và danh sách chân cần đo lại).

### Relay — ranh giới của tuyên bố

Firmware chỉ điều khiển **tín hiệu kích relay**; hàm được đặt tên `hal_relay_assert()` / `hal_relay_release()`, không dùng tên kiểu "cắt gas". Trạng thái thực tế của van phụ thuộc loại van (NC/NO), cách đấu tiếp điểm (NO/NC/COM) và nguồn cấp cho van — **không có thông tin nào về ba thứ đó trong báo cáo**. Vì vậy:

- Tài liệu này **không tuyên bố** hệ thống fail-safe khi mất điện.
- Kiểm thử chỉ đấu relay với **tải mô phỏng (LED + điện trở)**, không nối van gas thật.

---

## 3. Thuật toán ước lượng nồng độ LPG

### 3.1. Năm điện áp khác nhau — không được coi là bằng nhau

| Đại lượng | Giá trị dùng trong firmware | Bằng chứng |
|---|---|---|
| Điện áp cấp vào chân VCC của module MQ-2 | 3,3 V | **VERIFIED HARDWARE** |
| Điện áp gia nhiệt bên trong cảm biến | *không biết* | **UNVERIFIED** — không có sơ đồ mạch module |
| Điện áp cấp cho mạch đo (phân áp Rs–RL) bên trong module | giả thiết 3,3 V (`CFG_MQ2_VC_MV`) | **UNVERIFIED** |
| Điện áp AOUT | đo qua ADC | — |
| Vref của ADC | 3,3 V (`CFG_ADC_VREF_MV`) | **VERIFIED HARDWARE** |

**Giới hạn phải nêu rõ:** datasheet MQ-2 quy định điện áp gia nhiệt 5,0 V ± 0,1 V (**DATASHEET**). Hệ thống cấp 3,3 V vào module. Vì không có sơ đồ mạch của module PCB, không thể khẳng định điện áp thực tế trên cuộn gia nhiệt, cũng không thể khẳng định cảm biến đạt điều kiện làm việc mà đường đặc tuyến datasheet giả định. **Hệ quả: đường đặc tuyến chỉ là mô hình tham chiếu, và con số ppm là ước lượng tương đối chứ không phải nồng độ tuyệt đối đã hiệu chuẩn.** Đây là giới hạn của dữ liệu hiện có, không phải thứ sửa được bằng phần mềm — và theo phạm vi đã chốt, không thay đổi phần cứng để giải quyết.

### 3.2. Công thức

```
V_adc    = (raw / 4095.0) × V_REF                 , V_REF = 3300 mV
Rs       = RL × (V_C − V_adc) / V_adc             , V_C = 3300 mV, RL = 1000 Ω
ratio    = Rs / R0                                , R0 = 95.6 Ω
ppm_LPG  = 574.25 × ratio^(−2.222)
         ≡ 10^((log10(ratio) − 1.241721) / (−0.450045))
```

Vì AOUT nối trực tiếp vào PA0 (không phân áp), `V_adc` chính là `V_out` của mạch đo — công thức phân áp áp dụng trực tiếp được. Nếu sau này có chèn mạch phân áp, phải sửa cả hệ số chia lẫn `RL` hiệu dụng; hiện tại `CFG_MQ2_DIV` không tồn tại chính vì không có mạch đó.

Tất cả hằng số nằm trong `c8t6_producer/Inc/config.h`.

### 3.3. Xuất xứ của hệ số đặc tuyến

- `R0 = 95.6 Ω` lấy **từ chính mã nguồn trong báo cáo gốc** (§3.1.1 mục b, `float R0 = 95.6;`). Không tự thay bằng giá trị khác. **Bằng chứng: VERIFIED — mã nguồn bản gốc.**
- Giá trị này **tự nhất quán** với phương pháp hiệu chuẩn ở Phụ lục C: `R0 = Rs_air / 9.5` ⟹ `Rs_air = 908,2 Ω`, là trị số hợp lý với RL = 1 kΩ ở 3,3 V (tương ứng ADC ≈ 2146). Sự nhất quán này là lý do giữ nguyên 95,6 thay vì chọn số khác. **CALCULATED**, có unit test đối chiếu.
- Hệ số `9.5` là tỉ số `Rs/R0` trong không khí sạch theo đồ thị đặc trưng của MQ-2. **DATASHEET.**
- Cặp `A = 574.25`, `B = −2.222` là đường fit LPG dạng lũy thừa được dùng rộng rãi trong các thư viện MQ-2 và khớp với hệ số `m = −0.45` trong mã nguồn gốc (vì `1/(−2.222) = −0.450045`). **Phải nói rõ: đây là hệ số fit lưu hành trong cộng đồng, KHÔNG phải hệ số do Winsen công bố chính thức.** Winsen chỉ công bố đồ thị, không công bố cặp số này. Nhãn: **UNVERIFIED về xuất xứ chính thức**, nhưng **VERIFIED về tính nhất quán nội bộ** với hệ số của bản gốc.
- Phép quy đổi giữa hai dạng: `b = log10(574.25) / 2.222 = 1.241721`, `m = −1/2.222 = −0.450045`. Dùng nhầm `b = 1.2577` gây lệch khoảng 9 % — đủ để sai một mức cảnh báo. Có unit test chặn lỗi này. **CALCULATED.**

### 3.4. Lọc nhiễu

Giữ đúng phương pháp của Phụ lục C, mỗi chu kỳ 1 s:

1. Lấy **21 mẫu ADC**, cách nhau 10 ms.
2. Quy đổi từng mẫu ra **Rs**.
3. **Lọc 3-sigma**: loại mẫu ngoài `[mean − 3σ, mean + 3σ]`.
4. **Trung vị** các mẫu còn lại.
5. Từ Rs đại diện mới tính `ratio` rồi `ppm`.

Khác biệt duy nhất so với bản gốc là **lọc trên miền Rs thay vì miền ppm**.

Về tính tương đương của bước trung vị, bản tài liệu trước khẳng định quá mạnh ("cho kết quả y hệt") và **điều đó chỉ đúng khi số mẫu là lẻ**. Chính xác hơn:

- `mq2_filter_rs()` trả `rs[valid / 2]`, tức là **trung vị trên** (upper median), không phải trung bình hai phần tử giữa.
- Với số mẫu **lẻ**, phần tử giữa là duy nhất nên mọi phép biến đổi đơn điệu đều giữ nó: kết quả y hệt bản gốc.
- Với số mẫu **chẵn** — xảy ra khi batch 21 mẫu bị loại một mẫu không hợp lệ hoặc một outlier — `ppm(Rs)` là hàm *giảm*, nên trung vị trên của Rs ánh xạ thành trung vị **dưới** của ppm, khác với trung vị trên của ppm mà bản gốc sẽ cho. Hai cách có thể cho ra hai mức cảnh báo khác nhau ở batch chuyển tiếp.

Đây là **[F14]** trong danh mục audit, mức P1, chưa sửa thuật toán. Việc cần làm là chốt quy ước cho trường hợp chẵn rồi mô tả và kiểm thử nó như một thuật toán riêng, không mặc định phép biến đổi phi tuyến vẫn giao hoán.

Bước 3-sigma thì khác thật: nó dùng mean và standard deviation của chính batch, nên lọc trên Rs hay trên ppm cho kết quả khác nhau. Lý do chọn Rs là ppm là hàm mũ làm phân bố bị kéo lệch — đây là **lập luận về dạng phân bố, chưa có dữ liệu nhiễu thực tế của module để chứng minh**.

Ghi chú trung thực: với n = 21, quy tắc 3-sigma gần như không loại được mẫu nào (ngưỡng quá rộng cho cỡ mẫu nhỏ) — phần lớn công việc chống nhiễu thực chất do **trung vị** đảm nhiệm. Cả hai bước đều được giữ vì báo cáo đã đặc tả như vậy và chi phí không đáng kể, nhưng **không được quảng cáo 3-sigma là yếu tố chống nhiễu chính**. `test_host/test_mq2.c` có một ca kiểm thử minh họa đúng tình huống đó.

Thêm một lớp bảo vệ không có trong bản gốc: nếu quá nửa số mẫu trong một chu kỳ không hợp lệ, cả chu kỳ bị coi là lỗi cảm biến thay vì cho ra một con số.

### 3.5. Hiệu chuẩn R0

**Không tự động ghi đè R0 mỗi lần boot.** Môi trường lúc cấp nguồn có thể đã có khí; hiệu chuẩn vô điều kiện sẽ "học" luôn nồng độ đó làm mốc không khí sạch và khiến hệ thống mù vĩnh viễn với đúng mức rò rỉ ấy.

| | |
|---|---|
| **Chạy bình thường** | Dùng `CFG_MQ2_R0_OHM = 95.6` cố định trong `config.h` (giá trị của bản gốc) |
| **Hiệu chuẩn** | `mq2_r0_from_rs_air()` là hàm độc lập, không được gọi tự động. Dùng cho nghiên cứu/kiểm thử, hoặc gọi thủ công khi muốn tính lại R0 từ Rs đo được trong không khí sạch. Kết quả chép vào `config.h` và build lại. |

**Không bổ sung jumper PB12** (đã bỏ theo yêu cầu). Theo datasheet, cảm biến mới xuất xưởng cần burn-in 24–48 h trước khi hiệu chuẩn lần đầu (**DATASHEET**).

Firmware vẫn có **warm-up 30 s** sau mỗi lần cấp nguồn: trong thời gian đó hệ thống ở trạng thái `P_WARMUP`, không phát cảnh báo gas.

### 3.6. Giới hạn độ chính xác

- MQ-2 **không có tính chọn lọc khí** (**DATASHEET**): cồn, khói, CO, hơi dung môi đều làm Rs giảm. Con số ppm chỉ là "LPG tương đương".
- Chưa hiệu chuẩn bằng khí chuẩn → **sai số tuyệt đối không xác định được**. Con số ±10–15 % trong báo cáo là độ lặp lại của cảm biến, không phải sai số của phép quy đổi.
- Điện áp gia nhiệt chưa xác minh (mục 3.1) làm đường đặc tuyến datasheet chỉ còn giá trị tham chiếu.
- Rs phụ thuộc mạnh vào nhiệt độ và độ ẩm; V2 **không** bù nhiệt/ẩm.
- **CALCULATED — độ phân giải ở vùng cảnh báo:** với RL = 1 kΩ, V_C = 3,3 V và R0 = 95,6 Ω, dải 1000 → 2000 ppm chỉ ứng với ADC ≈ 3811 → 3883, tức khoảng **72 LSB cho cả 1000 ppm** (≈ 14 ppm/LSB). Vùng cảnh báo nằm ở đoạn gần bão hòa của phân áp nên độ phân giải kém và rất nhạy với nhiễu — đây chính là lý do bước lọc trung vị không thể bỏ. Trong khi đó không khí sạch ứng với ADC ≈ 2146, nên dải đo vẫn dùng được.
- Dưới 1000 ppm **không đồng nghĩa với an toàn**.

---

## 4. Ngưỡng cảnh báo

### 4.1. Cơ sở

Phải phân biệt bốn thứ, không gộp làm một:

| Nội dung | Nhãn |
|---|---|
| LPG chủ yếu gồm propane và butane; LEL propane ≈ 21 000 ppm, butane ≈ 19 000 ppm, LPG tham chiếu ≈ 20 000 ppm (phụ thuộc thành phần thực tế) | **DATASHEET** (dữ liệu an toàn công bố) |
| `ppm = (%LEL / 100) × LEL_ppm` → 5 % = 1000, 7,5 % = 1500, 10 % = 2000 ppm | **CALCULATED** |
| Chọn đúng ba mốc 5 % / 7,5 % / 10 % để chia bốn cấp | **DESIGN DECISION** của dự án |
| Khả năng đo và độ chính xác thực tế của MQ-2 trong hệ thống này | **UNVERIFIED** (mục 3.6) |
| Chứng nhận an toàn của một thiết bị hoàn chỉnh | **không có** — hệ thống này không được chứng nhận |

EN 50194-1 quy định điểm kích hoạt của thiết bị gia dụng nằm trong khoảng trên 3 % và dưới 20 % LFL, nên ba mốc trên nằm trong khung đó; nhưng **tiêu chuẩn không bắt buộc chia đúng ba mức này** và tài liệu này không tuyên bố tuân thủ tiêu chuẩn.

### 4.2. Bảng phân mức

| Mức | LPG | Ý nghĩa |
|---|---|---|
| 0 | < 1000 ppm | Chưa vượt ngưỡng cảnh báo thiết kế |
| 1 | 1000 – 1499 ppm | Cảnh báo sớm |
| 2 | 1500 – 1999 ppm | Cảnh báo cao |
| 3 | ≥ 2000 ppm | Cảnh báo nghiêm trọng |

Mức 1 **không** có nghĩa đã đạt giới hạn cháy dưới. Mức 0 **không** có nghĩa an toàn tuyệt đối.

### 4.3. Hysteresis

| Chuyển | Lên (ngay lập tức) | Xuống (cần 3 chu kỳ liên tiếp) |
|---|---|---|
| 0 ↔ 1 | ppm ≥ 1000 | ppm < 900 |
| 1 ↔ 2 | ppm ≥ 1500 | ppm < 1350 |
| 2 ↔ 3 | ppm ≥ 2000 | ppm < 1800 |

Hysteresis = 10 % ngưỡng. **Leo thang tức thì, hạ cấp có trễ** — bất đối xứng có chủ ý, vì chậm báo nguy hiểm hơn chậm thôi báo. Tính hợp lệ của thứ tự ngưỡng (`0 < TH1 < TH2 < TH3` và `TH_n − HYST_n > TH_{n−1}`) được kiểm tra trong unit test.

---

## 5. State machine

### 5.1. Producer (F103)

| Trạng thái | Điều kiện vào | Hành vi | Mã gửi đi |
|---|---|---|---|
| `P_WARMUP` | Cấp nguồn, trong 30 s đầu | Không phân mức. LCD: `WARMING UP nnn s` | `SW` mỗi 1 s |
| `P_RUN` | Hết warm-up và đang bật | Đo 1 s/lần, phân mức, cập nhật LCD | `S0`…`S3` |
| `P_STOPPED` | Nhận `O` | Ngừng đo, LCD `SysS:0` | `SO` mỗi 1 s |
| `P_SENSOR_FAULT` | ADC hoặc tỉ số ngoài dải hợp lệ trong **5 chu kỳ liên tiếp** | LCD: `SENSOR ERROR` | `SF` mỗi 1 s |

Thoát `P_SENSOR_FAULT` cần 5 chu kỳ hợp lệ liên tiếp — không thoát tức thì. Điều kiện lỗi: `raw < 20`, `raw > 4075`, hoặc `ratio ∉ [0.01, 50]`.

Producer **tự chạy độc lập** khi chưa nghe thấy Consumer (mặc định đang bật), và vẫn tiếp tục đo khi mất liên kết — chỉ hiện `L?` trên LCD.

### 5.2. Consumer (F401)

| Trạng thái | Điều kiện vào | Blue PB0 | Yellow PB1 | Red PB2 | Green PB3 | Buzzer | Relay |
|---|---|---|---|---|---|---|---|
| `C_SYS_OFF` | Công tắc tắt | — | — | — | **sáng** | — | nhả |
| `C_WARMUP` | nhận `SW` hoặc `SO` khi đang bật | **nháy 1 Hz** | — | — | — | — | nhả |
| `C_L0` | nhận `S0` | **sáng** | — | — | — | — | nhả |
| `C_L1` | nhận `S1` | — | **sáng** | — | — | — | nhả |
| `C_L2` | nhận `S2` | — | — | **nháy 1 Hz** | — | **liên tục** | **kích** |
| `C_L3` | nhận `S3` | — | — | **nháy 2→10 Hz tăng dần** | — | **liên tục** | **kích** |
| `C_SENSOR_FAULT` | nhận `SF` | — | **nháy 2 Hz** | **nháy 2 Hz lệch pha** | — | **bíp 100 ms / 2 s** | **kích** |
| `C_LINK_LOST` | không có khung hợp lệ trong 3 s, **hoặc mã lạ** | **nháy 1 Hz** | **nháy 1 Hz** | **nháy 1 Hz** (đồng pha) | — | **bíp 100 ms / 2 s** | **kích** |

Thứ tự ưu tiên: `SYS_OFF` > `LINK_LOST` > mã trạng thái.

**Nguyên tắc an toàn đã cài đặt và đã kiểm thử:**

- `C_SYS_OFF` và `C_L0` là hai trạng thái khác nhau.
- Lỗi cảm biến, mất UART và **mã không hợp lệ** đều không được coi là môi trường an toàn: cả ba kích tín hiệu relay và dùng mẫu âm thanh ngắt quãng, phân biệt được với còi liên tục của cảnh báo gas.
- `C_SYS_OFF` ghi đè mọi trạng thái khác và nhả relay. Đây là đánh đổi có chủ ý: người vận hành chủ động tắt giám sát thì hệ thống không tự giữ relay. **Phải ghi rõ trong hướng dẫn sử dụng rằng tắt công tắc là tắt bảo vệ.**

Phần quyết định của state machine nằm trong `ccu6_consumer/Src/alarm_fsm.c`, thuần tính toán, không chạm phần cứng, và được kiểm thử trực tiếp trên PC.

### 5.3. Đồng bộ khi khởi động lại

Xem `PROTOCOL.md` mục 7.

---

## 6. Clock và ngoại vi

### 6.1. STM32F103C8T6

| Mục | Giá trị |
|---|---|
| Nguồn | HSE 8 MHz → PLL ×9 |
| SYSCLK / AHB / APB1 / APB2 | 72 / 72 / 36 / 72 MHz |
| Flash latency | 2 WS, bật prefetch (**DATASHEET** RM0008) |
| Fallback | HSE không lên sau 100 000 vòng → HSI 8 MHz, cờ `clock_degraded`, LCD hiện `HS` |

| Ngoại vi | @72 MHz | Fallback @8 MHz |
|---|---|---|
| USART1 (APB2) | `BRR = 7500` | `BRR = 833` |
| USART2 (APB1) | `BRR = 3750` | `BRR = 833` |
| ADC1 | ADCPRE = /6 → **12 MHz** (≤ 14 MHz), sample time 239,5 cy | ADCPRE = /2 → 4 MHz |
| I2C1 | Standard mode 100 kHz, `CCR`/`TRISE` suy từ PCLK1 | như trên |
| SysTick | 1 ms | như trên |

Mọi giá trị BRR/CCR/PSC đều **tính tại runtime** từ `bsp_pclk1_hz()` / `bsp_pclk2_hz()`, nên cả chế độ chính lẫn fallback đều đúng. Không còn hằng số viết tay như bản gốc.

### 6.2. STM32F401CCU6

| Mục | Giá trị |
|---|---|
| Nguồn | HSE 25 MHz → PLL M=25, N=336, P=4, Q=7 |
| SYSCLK / AHB / APB1 / APB2 | 84 / 84 / 42 / 84 MHz |
| Flash latency | 2 WS + prefetch + I-cache + D-cache (**DATASHEET** RM0368) |
| Fallback | HSE lỗi → HSI 16 MHz, cờ `clock_degraded` |
| Nếu thạch anh là 8 MHz | chỉ sửa `CFG_HSE_HZ`; `CFG_PLL_M` tự suy để VCO input = 1 MHz |

| Ngoại vi | @84 MHz | Fallback @16 MHz |
|---|---|---|
| USART2 (APB1 = 42 MHz) | `BRR = 4375` | `BRR = 1667` |
| TIM3 (clock 84 MHz) | `PSC = 8399` → tick 100 µs, `ARR = 9` → 1 ms | `PSC = 1599` → tick 100 µs |
| EXTI13 | SYSCFG `EXTICR[3]` → Port C; IMR13 + RTSR13 + FTSR13 | — |

**TIM3 làm 2 việc** (gộp để không tốn thêm timer): nhịp 1 ms cho mẫu nháy LED / mẫu bíp buzzer, và đếm lùi cửa sổ chống rung công tắc.

### 6.3. Mức ưu tiên ngắt

| Ngắt | Priority | Lý do |
|---|---|---|
| SysTick | 0 | Timebase |
| USART RX | 1 | Mất byte là mất trạng thái |
| TIM3 | 2 | Trễ 100 µs không ảnh hưởng |
| EXTI15_10 | 3 | Thao tác của người, trễ vài ms không ai thấy |

Mọi ISR chỉ đẩy byte vào ring buffer hoặc đặt cờ. **Không vòng lặp trễ, không `sprintf`, không tính dấu phẩy động trong ISR.**

### 6.4. Tiết kiệm năng lượng

Giữ `__WFI()` như bản gốc nhưng sửa đúng cửa sổ chạy đua: Consumer kiểm tra cờ "có việc cần làm" khi ngắt đang tắt rồi `__WFI()` ngay trước khi bật lại ngắt. Bản gốc đọc cờ rồi bật lại ngắt trước khi gọi `__WFI()`, nên một sự kiện đến đúng khe đó sẽ bị bỏ lỡ.

---

## 7. Cấu trúc module

```
bao_chay/
├─ docs/           DESIGN_SPEC_V2.md · PROTOCOL.md · PIN_MAPPING.md
├─ c8t6_producer/  Drivers/CMSIS · Inc · Src · Startup · Makefile
├─ ccu6_consumer/  Drivers/CMSIS · Inc · Src · Startup · Makefile
├─ esp32_blynk/    esp32_blynk.ino · config.h · secrets_example.h
├─ test_host/      test_mq2.c · test_protocol.c · test_alarm_fsm.c · test_ring.c · test_debounce.c · Makefile
├─ Makefile        build + test toàn bộ
└─ README.md
```

**Bốn quy tắc tổ chức:**

1. `mq2_sensor.c`, `protocol.c`, `alarm_fsm.c`, `debounce.c`, `ring.h` **không include header thanh ghi nào** → biên dịch và kiểm thử được trên PC. Unit test dùng chính các file đó, không phải bản mô phỏng.
2. Lớp `drv_*` là nơi duy nhất chạm thanh ghi. Lớp `hal_*` gói ý nghĩa ứng dụng.
3. Các tham số hành vi nằm trong `config.h`; implementation vẫn chứa bitmask, mã lệnh LCD và hằng số định dạng. Câu cũ “không có số ma thuật trong `.c`” là quá tuyệt đối.
4. `Src/protocol.c`, `Inc/protocol.h`, `Inc/ring.h`, `Inc/uart_rx_event.h` có hai bản giống hệt ở hai project (CubeIDE không có thư mục dùng chung). `make check-sync` ép hai bản phải khớp, nếu không thì build đổ.

Danh sách file đầy đủ: xem `README.md`.

---

## 8. Kiểm chứng

| Tầng | Nội dung | Trạng thái |
|---|---|---|
| 1 | Unit test trên PC (`make test`) — quy đổi ppm, tương đương hai dạng đặc tuyến, lọc, phân mức, hysteresis, parser UART, timeout, state machine, ràng buộc an toàn của đầu ra | **ĐÃ CHẠY — 1009/1009 đạt** |
| 2 | Biên dịch hai firmware bằng `arm-none-eabi-gcc` với `-Wall -Wextra -Werror -Wshadow -Wconversion` | **ĐÃ CHẠY — sạch** |
| 2b | Build bằng chính STM32CubeIDE 2.2.0 qua `.cproject` (headless, cả Debug và Release của cả hai project) | **ĐÃ CHẠY — 4/4 cấu hình, 0 error, 0 warning** |
| 2c | Biên dịch sketch ESP32 bằng `arduino-cli 1.5.1`, core `esp32:esp32@3.3.12`, `Blynk@1.4.0`, `--warnings all` | **ĐÃ CHẠY — 0 error, 0 warning; 70 % flash, 14 % RAM** |
| 2d | `make check-sync` — bốn file dùng chung giữa hai project phải khớp | **ĐÃ CHẠY — khớp**; chốt đã thử cả hai chiều |
| 3 | Kiểm thử trên bàn từng board | **CHƯA** — không có phần cứng |
| 4 | Tích hợp ba thiết bị | **CHƯA** — không có phần cứng |

Chi tiết tầng 3–4 (để dành khi có linh kiện): nạp firmware, gõ tay `S0`…`S3`/`SF`/`SW` qua USB-TTL vào PA2/PA3 của F401 để nghiệm thu bảng 6.2; ngừng gửi để kiểm tra timeout 3 s; thay MQ-2 bằng biến trở 10 kΩ để quét toàn dải ADC; đấu relay với LED + điện trở 1 kΩ thay cho van gas; bật `CFG_SIMULATE_PPM` trên F103 để quét ppm 0 → 2600 mà không cần cảm biến.

---

## 9. Những gì tài liệu này KHÔNG tuyên bố

1. **Không** tuyên bố hệ thống là thiết bị an toàn đã được chứng nhận.
2. **Không** tuyên bố ngưỡng 1000/1500/2000 ppm là điểm kích hoạt đúng — chưa hiệu chuẩn bằng khí chuẩn.
3. **Không** tuyên bố cơ chế fail-safe khi mất điện — chưa có sơ đồ mạch relay, chưa biết loại van.
4. **Không** tuyên bố con số ppm là nồng độ LPG tuyệt đối — MQ-2 không chọn lọc khí và điện áp gia nhiệt chưa xác minh.
5. **Không** tuyên bố hệ số `574.25 / −2.222` là số liệu chính thức do Winsen công bố.
6. **Không** tuyên bố đã kiểm thử trên phần cứng — chưa có linh kiện để lắp.
7. **Không** thay thế đầu báo gas dân dụng đạt chuẩn.

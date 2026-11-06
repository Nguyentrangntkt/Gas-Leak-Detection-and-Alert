# Bảng chân kết nối

In trang này ra để đối chiếu khi lắp mạch.

Phân loại bằng chứng: **VERIFIED HARDWARE** (nhóm đã xác nhận) · **DATASHEET** · **CALCULATED** · **DESIGN DECISION** (chọn cho bản khôi phục) · **UNVERIFIED** (chưa đo/kiểm).

---

## STM32F103C8T6 — Producer

| Ngoại vi | Chân | Chức năng | Chế độ | Bằng chứng |
|---|---|---|---|---|
| MQ-2 AOUT | PA0 | ADC1_IN0 | Analog input, sample time 239,5 chu kỳ | VERIFIED HARDWARE |
| LCD 16x2 I2C | PB6 | I2C1_SCL | AF open-drain 50 MHz | VERIFIED HARDWARE (chân) |
| LCD 16x2 I2C | PB7 | I2C1_SDA | AF open-drain 50 MHz | VERIFIED HARDWARE (chân) |
| → F401 | PA9 | USART1_TX | AF push-pull 50 MHz | VERIFIED HARDWARE |
| ← F401 | PA10 | USART1_RX | Input **kéo lên** (CNF=10, ODR10=1) | VERIFIED HARDWARE (chân) |
| → ESP32 | PA2 | USART2_TX | AF push-pull 50 MHz | VERIFIED HARDWARE |
<!-- PA10 kéo lên (không phải thả nổi): rút dây liên kết thì chân thả nổi
     bắt nhiễu, sinh lỗi khung và byte rác liên tục. Kéo lên giữ đường ở
     mức idle của UART, nên mất dây = im lặng và hai đầu phát hiện bằng
     timeout. Xem F21 trong README mục 9.1. Nhãn bằng chứng là "chân" vì
     số chân lấy từ báo cáo, còn cấu hình kéo lên là quyết định của bản
     khôi phục. -->
| (chưa dùng) | PA3 | USART2_RX | Không cấu hình | — |
| Nạp/debug | PA13/PA14 | SWDIO/SWCLK | Không dùng cho việc khác | DATASHEET |

**Chân phải tránh trên F103:** PB2 = BOOT1 (có điện trở trên board Blue Pill) · PA11/PA12 = USB · PC13 = LED onboard của Blue Pill.

---

## STM32F401CCU6 — Consumer

| Ngoại vi | Chân | Mức tích cực | Bằng chứng |
|---|---|---|---|
| LED xanh dương | PB0 | HIGH (R 220 Ω xuống GND) | VERIFIED HARDWARE (Hình 3) |
| LED vàng | PB1 | HIGH | VERIFIED HARDWARE (Hình 3) |
| LED đỏ | PB2 | HIGH | VERIFIED HARDWARE (Hình 3) |
| LED xanh lá | PB3 | HIGH | VERIFIED HARDWARE (Hình 3) |
| Buzzer | PA1 | HIGH | **DESIGN DECISION** + cực tính UNVERIFIED |
| Tín hiệu relay | PB5 | **LOW** | **DESIGN DECISION** (chân) · cực tính VERIFIED |
| Công tắc Start/Stop | PC13 | LOW = ON, pull-up nội, EXTI13 hai sườn | VERIFIED HARDWARE (Hình 4) |
| Nút Reset | NRST | LOW, reset cứng — không cần code | VERIFIED HARDWARE (Hình 4) |
| → F103 | PA2 | USART2_TX, AF7 | VERIFIED HARDWARE |
| ← F103 | PA3 | USART2_RX, AF7, pull-up | VERIFIED HARDWARE |
| Nạp/debug | PA13/PA14 | SWDIO/SWCLK | DATASHEET |

### Cảnh báo chân đặc biệt (DATASHEET)

- **PB2 = BOOT1.** Chỉ được lấy mẫu tại thời điểm reset và chỉ có tác dụng khi BOOT0 = 1, nên dùng làm output sau boot là hợp lệ. Nếu board có điện trở BOOT1 thì dòng qua LED đỏ bị ảnh hưởng nhẹ — nên đo lại khi lắp.
- **PB3 = JTDO, PB4 = NJTRST.** Sau reset F401 bật sẵn JTAG+SW. Đặt PB3 thành GPIO output làm **mất khả năng debug qua JTAG**; SWD qua PA13/PA14 vẫn hoạt động bình thường.
- **PA0 = WKUP** — đó là lý do buzzer dùng PA1 chứ không phải PA0.
- **PB5** trống hoàn toàn trên F401CCU6.

### PA1 và PB5 — chưa xác minh

Báo cáo gốc chỉ ghi buzzer ở GPIOA và relay ở GPIOB, **không ghi số chân**. PA1 và PB5 là lựa chọn cho bản firmware khôi phục, không phải chân đã dùng trên phần cứng gốc. Nếu mạch đã hàn theo chân khác, sửa `CFG_BUZZER_PIN` và `CFG_RELAY_PIN` trong `ccu6_consumer/Inc/config.h`.

---

## ESP32

| Ngoại vi | Chân | Bằng chứng |
|---|---|---|
| ← F103 (UART2 RX) | GPIO16 | **DESIGN DECISION** (báo cáo không ghi chân) |
| GND | GND chung với cả hai STM32 | **bắt buộc** |

---

## MQ-2

| Chân | Nối với | Bằng chứng |
|---|---|---|
| VCC | **3,3 V** | VERIFIED HARDWARE |
| GND | GND chung | VERIFIED HARDWARE |
| AOUT | PA0 của F103 (nối trực tiếp, không phân áp) | VERIFIED HARDWARE |
| DOUT | không dùng | — |

### Giới hạn phải ghi nhận (xem thêm DESIGN_SPEC_V2 mục 4)

Datasheet MQ-2 quy định điện áp gia nhiệt 5,0 V ± 0,1 V. Hệ thống cấp 3,3 V vào chân VCC của module. Vì **không có sơ đồ mạch của module PCB**, không thể khẳng định điện áp thực tế đặt lên cuộn gia nhiệt và lên mạch đo bên trong bằng đúng 3,3 V, cũng không thể khẳng định nó đạt điều kiện làm việc của datasheet. Do đó đường đặc tuyến datasheet chỉ dùng làm **mô hình tham chiếu**, và con số ppm là ước lượng tương đối chứ không phải nồng độ tuyệt đối đã hiệu chuẩn.

---

## Những điểm phải đo lại khi có phần cứng (VERIFICATION)

| # | Nội dung | Cách kiểm |
|---|---|---|
| V1 | Thạch anh F401 là 25 MHz hay 8 MHz | Đọc chữ in trên thạch anh; hoặc nháy LED 1 Hz rồi bấm giờ 60 s |
| V2 | Thạch anh F103 là 8 MHz | Như trên |
| V3 | RL thực trên module MQ-2 (1 kΩ / 5 kΩ / có biến trở) | Đo bằng đồng hồ khi chưa cấp nguồn |
| V4 | Địa chỉ I2C của LCD (0x27 hay 0x3F) | Chạy I2C scan một lần |
| V5 | Chân buzzer và relay thực tế | Dò mạch đã hàn |
| V6 | PB2 (BOOT1) có điện trở trên board không | Đo dòng LED đỏ khi sáng |
| V7 | Dòng tiêu thụ khi relay + buzzer + 2 LED cùng hoạt động | Nguồn có hiển thị dòng |

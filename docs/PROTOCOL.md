# Giao thức UART

Đặc tả khung truyền giữa ba thiết bị. Bản cài đặt nằm ở `protocol.c` / `protocol.h`, có hai bản sao đồng bộ trong `c8t6_producer/` và `ccu6_consumer/`. `make check-sync` ép hai bản phải khớp và làm đổ build nếu chúng lệch nhau.

## 1. Tham số vật lý

| Liên kết | Cổng | Chân | Tham số |
|---|---|---|---|
| F103 ↔ F401 | F103 USART1 ↔ F401 USART2 | PA9/PA10 ↔ PA2/PA3 (chéo TX–RX) | 9600 8N1 |
| F103 → ESP32 | F103 USART2 → ESP32 UART2 | PA2 → GPIO16 | 9600 8N1, một chiều |

Mức logic 3,3 V. **GND của cả ba board phải nối chung** — thiếu dây GND thì liên kết không bao giờ hoạt động. Báo cáo gốc có nêu điều này ở trang 20 và 26.

## 2. Khung F103 → F401 (trạng thái)

```
'S' <code> '\n'          3 byte cố định
```

| `code` | Ý nghĩa |
|---|---|
| `'0'` | Dưới ngưỡng cảnh báo thiết kế (< 1000 ppm) |
| `'1'` | Cảnh báo sớm (≥ 1000 ppm) |
| `'2'` | Cảnh báo cao (≥ 1500 ppm) |
| `'3'` | Cảnh báo nghiêm trọng (≥ 2000 ppm) |
| `'W'` | Chưa sẵn sàng: đang làm nóng hoặc chờ phép đo mới sau Start |
| `'F'` | Lỗi cảm biến (ADC hoặc tỉ số ngoài dải hợp lệ) |
| `'O'` | Hệ thống đã dừng theo lệnh `O` |

Tần suất: gửi **ngay khi `code` đổi**, và **lặp lại mỗi 1000 ms** làm heartbeat.

## 3. Khung F401 → F103 (lệnh)

```
'I' '\n'     start — công tắc bật
'O' '\n'     stop  — công tắc tắt
```

Gửi khi mức công tắc đã ổn định đủ 50 ms theo bộ chống rung được lấy mẫu mỗi 1 ms; gửi một lần lúc boot và lặp lại mỗi 5000 ms. EXTI chỉ đánh thức CPU, không quyết định trạng thái ổn định.

## 4. Khung F103 → ESP32 (telemetry)

```
'P' <ppm, 5 chữ số, đệm 0> ',' <code> '\n'      9 byte cố định
```

Ví dụ: `P01432,1\n`. `ppm` bão hòa ở 99999. `code` giống mục 2. Mỗi 1000 ms, một chiều, ESP32 không phản hồi.

## 5. Quy tắc phân tích

Bộ phân tích gom byte đến khi gặp `'\n'`, bỏ qua `'\r'` nên chịu được cả CRLF. Một dòng bị loại hoàn toàn khi:

- dài quá `PROTO_MAX_LINE` (12 byte) — bộ đệm đánh dấu tràn và vẫn chạy tới `'\n'` kế tiếp để tái đồng bộ;
- rỗng;
- sai độ dài, sai ký tự dẫn đầu, hoặc `code` không nằm trong tập hợp lệ.
- trên hai STM32: có sự kiện `UART_RX_GAP` do ORE/NE/FE/PE hoặc tràn ring RX. Driver xả queue trong đoạn khóa ngắt ngắn, giữ nguyên PRIMASK của bên gọi; foreground gọi `proto_rx_discard()` rồi bỏ tới newline mới. ISR không sửa bộ phân tích dòng.

**Dòng bị loại không cập nhật mã nhận hoặc timestamp khung hợp lệ.** Timeout vẫn có thể làm thay đổi trạng thái. Test parser, driver UART thật với thanh ghi giả lập và ứng dụng Consumer kiểm tra ba lớp riêng biệt.

Xả queue là lựa chọn bảo thủ: newline đang nằm trong queue có thể xuất hiện **trước** byte bị mất, nên không dùng nó làm mốc đồng bộ. Có thể mất thêm khung lành; heartbeat hoặc lệnh lặp sẽ khôi phục khi đường truyền tốt trở lại. API sự kiện này áp dụng cho F103/F401; sketch ESP32 hiện chỉ loại lỗi cú pháp/tràn bộ đệm dòng, chưa có đường thông báo lỗi UART phần cứng tương đương.

## 6. Timeout

| Phía | Timeout | Hành động |
|---|---|---|
| F401 | 3000 ms không có khung `S` hợp lệ | → `C_LINK_LOST`: kích relay, LED nháy đồng pha, buzzer bíp ngắt quãng |
| F103 | 15000 ms không có lệnh `I`/`O` | Cờ `link_down`, LCD hiện `L?`. **Vẫn tiếp tục đo và gửi** |
| ESP32 | 5000 ms không có khung `P` | Báo offline lên V2, không tạo cảnh báo giả |

## 7. Đồng bộ lại

| Tình huống | Cơ chế |
|---|---|
| F401 boot trước F103 | F401 ở `C_LINK_LOST` tới khung đầu tiên |
| F103 boot trước F401 | F103 mặc định tự chạy, đồng bộ theo lệnh `I`/`O` đầu tiên |
| F401 reset giữa chừng | Đọc PC13 lúc boot → gửi `I`/`O` → F103 trả **trạng thái hiện tại** ngay |
| Bật lại sau Stop | F401 gửi `I`; F103 chuyển trạng thái và trả lời **ngay trong cùng vòng lặp**, không đợi hết chu kỳ 1 s |

Về khung trả lời tức thì cho lệnh `I`: nó mang **trạng thái thật ở thời điểm đó**, không phải một phép đo mới.

| Tình trạng khi nhận `I` | Mã trả về ngay |
|---|---|
| Còn trong warm-up | `W` |
| Lỗi cảm biến đã xác nhận (latch sống qua Stop) | `F` |
| Chưa có số đo đủ mới (vừa Start sau khi Stop lâu) | `W` — chưa sẵn sàng, **không** phải `F` và cũng không phải `0` |
| Có số đo đủ mới | `0`…`3` |

Phép đo mới mất khoảng 200 ms cho cả batch 21 mẫu, nên F103 **không** đo ngay trong đường xử lý RX; số đo thật đến ở nhịp kế tiếp. Trước đây tài liệu này hứa "gửi mức ngay" trong khi mã gửi lại `O` — xem F09 ở README mục 9.1.

Khung đổi mức bị mất được bù bằng heartbeat khi khung sau tới nguyên vẹn. Sau GAP, bộ thu có thể bỏ thêm một dòng để đồng bộ; không hứa mọi trường hợp phục hồi trong đúng 1 s.

## 8. Vì sao không dùng CRC

**DESIGN DECISION, 05/11/2026:** chủ dự án yêu cầu giữ giao thức hiện tại trong lượt sửa này; không thêm CRC/sequence vào ba đầu. Độ ngắn của khung và heartbeat không phải bằng chứng thống kê về xác suất phát hiện lỗi.

Giới hạn: đổi một bit `'2' ^ 0x02 == '0'` biến `S2` thành `S0`, vẫn đúng cú pháp. Nếu USART không báo lỗi, bộ thu có thể chấp nhận và nhả cảnh báo tới khung đúng kế tiếp hoặc timeout. GAP chỉ xử lý **lỗi đã được phát hiện**, không thay thế kiểm tra tính toàn vẹn. Parity cũng không thay CRC.

Đính chính lịch sử: bản trước viết “lỗi gần như luôn làm sai cú pháp” mà không có dữ liệu; ví dụ “một bit S1 thành S2” cũng sai vì XOR là 0x03, đổi hai bit. Cả hai nhận định đã được sửa.

## 9. Chống blocking

- **TX**: ring buffer theo `CFG_UART_TX_RING` (mặc định 64 ô, chứa tối đa 63 byte) + ngắt TXE. Không đủ chỗ cho nguyên khung → bỏ **nguyên khung**, không nạp byte nào, tăng `dropped`. Driver và test dùng chung `ring_write_frame()` / `ring_read_byte()`.
- **RX**: ngắt RXNE đẩy byte vào ring 32 byte; ghép dòng và phân tích làm ở vòng lặp chính.
- ISR đọc SR rồi DR khi ORE/NE/FE/PE, đếm lỗi, bỏ byte và đánh dấu GAP; RX đầy cũng đánh dấu GAP. Không chờ newline trong ISR.

Đính chính lịch sử: câu cũ “một lần overrun là USART chết vĩnh viễn” không đúng. Bản gốc đọc DR khi RXNE nên có thể đã thực hiện chuỗi xóa cờ; lỗi là dùng byte không tin cậy và bỏ sót thông tin mất byte. Xem comment đầu hai `drv_uart.c` và bản audit gốc.

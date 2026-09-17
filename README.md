# Hệ thống giám sát và kiểm soát chất lượng không khí trong nhà
Đồ án xây dựng một hệ thống sử dụng ESP32 để giám sát nhiệt độ, độ ẩm và chất lượng không khí trong nhà. Dữ liệu được hiển thị trên màn hình OLED và gửi lên nền tảng Blynk thông qua Wi-Fi.
Hệ thống có khả năng điều khiển các thiết bị điện tự động theo ngưỡng cài đặt hoặc điều khiển thủ công bằng nút nhấn và Blynk.
## Chức năng chính
- Đo nhiệt độ và độ ẩm bằng cảm biến DHT11.
- Đo bụi bằng cảm biến GP2Y1010AU0F.
- Hiển thị dữ liệu trên màn hình OLED.
- Gửi dữ liệu lên Blynk qua Wi-Fi.
- Điều khiển 5 kênh relay.
- Hỗ trợ chế độ tự động và điều khiển thủ công.
- Cấu hình Wi-Fi, Blynk và các ngưỡng điều khiển thông qua giao diện web.
- Lưu thông tin cấu hình bằng EEPROM.
- Sử dụng các task FreeRTOS để xử lý cảm biến, hiển thị, mạng và điều khiển.
## Phần cứng
| Thành phần | Mô tả |
|---|---|
| Vi điều khiển | ESP32 ESP-WROOM-32S |
| Cảm biến nhiệt độ, độ ẩm | DHT11 |
| Cảm biến bụi | GP2Y1010AU0F |
| Màn hình | OLED 1.3 inch, 128×64, giao tiếp I2C |
| Bộ điều khiển | Relay 5V, 5 kênh |
| Nút nhấn | 5 nút nhấn cơ học |
| Nền tảng IoT | Blynk IoT |
## Sơ đồ chân kết nối
| Thiết bị | Chân ESP32 |
|---|---|
| DHT11 | GPIO 18 |
| GP2Y1010AU0F - điều khiển LED | GPIO 23 |
| GP2Y1010AU0F - ngõ ra analog | GPIO 36 |
| OLED - SDA | GPIO 21 |
| OLED - SCL | GPIO 22 |
| Relay 1 đến Relay 5 | GPIO 25, 26, 27, 14, 13 |
| Nút nhấn 1 đến nút nhấn 5 | GPIO 33, 32, 35, 34, 39 |
## Phần mềm sử dụng
- Arduino IDE 2.3.10
- Ngôn ngữ C/C++
- ESP32 Arduino Core
- FreeRTOS
- Wi-Fi và Blynk IoT
- AP Mode, WebServer và EEPROM
## Thư viện cần thiết
- Blynk
- DHT sensor library
- Adafruit GFX Library
- Adafruit SH110X
- AsyncTCP
- ESPAsyncWebServer
- Arduino_JSON
## Cách sử dụng
1. Cài Arduino IDE 2.3.10 và ESP32 Board Package.
2. Cài đặt các thư viện cần thiết.
3. Mở `code_DATN.ino`, đặt các file `.h` cùng thư mục.
4. Chọn board ESP32 và đúng cổng COM.
5. Kết nối ESP32 bằng USB và nạp chương trình.
6. Cấu hình Wi-Fi, Blynk và ngưỡng điều khiển trên giao diện Web.
## Cấu trúc mã nguồn
- code_DATN.ino: chương trình chính.
- data_config.h: dữ liệu và thông số cấu hình.
- icon.h: dữ liệu biểu tượng hiển thị trên OLED.
- index_html.h: giao diện web cấu hình ESP32.
## Tác giả

Ngô Văn Quý

Đồ án tốt nghiệp ngành Kỹ thuật máy tính.

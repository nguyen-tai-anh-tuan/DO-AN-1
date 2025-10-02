**Hệ Thống Giám Sát Nhà Thông Minh 🏠**

**Giới Thiệu 📖**

Dự án tập trung vào việc thiết kế và thi công một hệ thống giám sát nhà thông minh phân tán, nhằm giải quyết vấn đề chi phí cao và yêu cầu kết nối ổn định ở các hộ gia đình hiện đại. Hệ thống sử dụng hai vi điều khiển chính là STM32 và ESP32, kết hợp các cảm biến, module xác thực và giao tiếp đám mây để giám sát môi trường, xác thực người dùng, điều khiển thiết bị và cài đặt an ninh.

**Chức Năng Chính ⚙️**

**Giám sát môi trường:** Đo lường và hiển thị các thông số như nhiệt độ, độ ẩm (DHT22), cường độ ánh sáng (BH1750), khí gas (MQ-2). Dữ liệu được hiển thị trên màn hình LCD 20x4 qua giao tiếp I2C và truyền đến ESP32 qua UART.  
**Xác thực người dùng:** Hỗ trợ đa phương thức bao gồm mật khẩu (Keypad 4x4), thẻ RFID (RC522), và vân tay (AS608). Khi xác thực thành công, hệ thống điều khiển servo để mở cửa.  
**Điều khiển thiết bị:** Sử dụng module relay 4 kênh để bật/tắt các thiết bị gia dụng.  
**Giám sát từ xa:** ESP32 gửi dữ liệu lên Firebase Realtime Database để lưu trữ. Web Dashboard hiển thị dữ liệu thời gian thực, lịch sử, và cho phép điều khiển thiết bị qua giao diện web.  
**Cài đặt an ninh:** Chế độ cài đặt để cấu hình hệ thống, đảm bảo an toàn và tiện nghi.  

**Công Nghệ Sử Dụng 🔧**

**Vi điều khiển:** STM32F407 (xử lý cảm biến và điều khiển cục bộ), ESP32 (giao tiếp không dây và đám mây).  
**Giao tiếp:** UART (giao tiếp nội bộ), I2C (LCD và cảm biến), SPI (RFID), Wi-Fi (kết nối Firebase).  
**Nền tảng đám mây:** Firebase cho lưu trữ và đồng bộ dữ liệu thời gian thực.  
**Phần cứng khác:** Servo motor, module relay, nguồn cấp (5V/3.3V).  

**Thiết Kế Và Kết Quả 🛠️**

**Phần cứng:** Sơ đồ khối phân tán với các khối cảm biến, điều khiển, hiển thị, nhận diện và nguồn. Tổng công suất tiêu thụ khoảng 5W, phù hợp cho hệ thống gia đình.  
**Phần mềm:** Lưu đồ giải thuật cho các chế độ hoạt động, lập trình trên STM32 (C/C++) và ESP32 (Arduino IDE hoặc ESP-IDF).  
**Kết quả:** Hệ thống hoạt động ổn định, xác thực nhanh chóng (dưới 1 giây), giám sát chính xác. Web Dashboard hỗ trợ xem dữ liệu lịch sử và điều khiển từ xa.  
**Ưu điểm:** Chi phí thấp, linh hoạt, tích hợp đa cảm biến và xác thực.  
**Hạn chế:** Chưa hỗ trợ AI nâng cao hoặc tích hợp di động đầy đủ.  

**Hướng Phát Triển 🚀**

Tích hợp camera giám sát và nhận diện khuôn mặt.  
Hỗ trợ ứng dụng di động (iOS/Android) qua Bluetooth hoặc MQTT.  
Tối ưu hóa năng lượng và mở rộng cho các thiết bị IoT khác.  

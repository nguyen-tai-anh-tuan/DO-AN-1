/*
=== PROTOCOL STM32 - ESP32 (Đơn giản) ===
ESP32 -> STM32:
  'A' = Vào chế độ xác thực
  'B' = Di chuyển con trỏ trong menu
  '#' = Xác nhận lựa chọn
  'C' = Vào chế độ hiển thị cảm biến
  'D' = Vào chế độ setup (cài đặt)
  'W' = Yêu cầu xác thực mật khẩu để setup
  'I' = Quay về màn hình intro
  'O' = Xác thực thành công (mở cửa)
  'P' = Bắt đầu nhập mật khẩu (cho keypad)
  'R' = Bắt đầu xác thực RFID
  'V' = Bắt đầu xác thực vân tay
  'S' = Hiển thị menu setup
  'N' = Setup mật khẩu mới
  'G' = Setup vân tay mới
  'T' = Setup thẻ RFID mới
  'Y' = Setup thành công
  'E' = Lỗi setup
  '0'-'9' = Các số mật khẩu
  'X' = Xóa ký tự cuối
  'F' = Sai mật khẩu
  'U' = Lỗi xác thực RFID
  'Z' = Lỗi xác thực vân tay
  'k' và 'j' = bật/tắt relay 1
  'h' và 'g' = bật/tắt relay 2
  'f' và 'd' = bật/tắt relay 3
  's' và 'a' = bật/tắt relay 4
  
=== END PROTOCOL ===
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Adafruit_Fingerprint.h>
#include <FirebaseESP32.h>
#include <addons/RTDBHelper.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// NTP setup
const long utcOffsetInSeconds = 0; // UTC, sẽ điều chỉnh ở client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// RC522 setup
#define SS_PIN 5
#define RST_PIN 2
MFRC522 rfid(SS_PIN, RST_PIN);

#define RXD2 16
#define TXD2 17

// Thông tin Wi-Fi
//#define WIFI_SSID "Kingshouse-Tret1"
//#define WIFI_PASSWORD "kingshouse2018"

//#define WIFI_SSID "IoT Lab"         
//#define WIFI_PASSWORD "IoT@123456" 

#define WIFI_SSID "ZTE-e1b3f5"         
#define WIFI_PASSWORD "88d274e1"

#define DATABASE_URL "https://smart-home-d6e89-default-rtdb.firebaseio.com/" 
#define DATABASE_SECRET "AIzaSyAwNLS7RHasQrILtUGDW6BiNZqB5u9U5WY"
#define USER_EMAIL "nguyentaianhtuan2004@gmail.com"
#define USER_PASSWORD "0123456789"

#define PASSWORD_PATH "system/password"
#define SENSOR_PATH "sensors"
#define DEFAULT_PASSWORD "123456"

// Thresholds để kiểm tra thay đổi đáng kể
#define TEMP_THRESHOLD 0.3    // °C
#define HUMIDITY_THRESHOLD 0.5 // %
#define GAS_THRESHOLD 5       // đơn vị ADC
#define LIGHT_THRESHOLD 20    // lux

HardwareSerial mySerial(1);  // UART2 cho vân tay
HardwareSerial mySerial2(2); // UART2 cho STM32
Adafruit_Fingerprint finger(&mySerial);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;
int rl1, rl2, rl3, rl4;
// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 27, 14};
byte colPins[COLS] = {12, 13, 4, 15};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Biến trạng thái đơn giản
enum Mode {
  MODE_NORMAL,
  MODE_AUTH_PASSWORD,
  MODE_AUTH_RFID,
  MODE_AUTH_FINGERPRINT,
  MODE_VERIFY_SETUP,
  MODE_SETUP,
  MODE_SETUP_PASSWORD,
  MODE_SETUP_FINGERPRINT,
  MODE_SETUP_RFID
};

enum AuthType {
  AUTH_KEYPAD = 0,
  AUTH_RFID = 1,
  AUTH_FINGERPRINT = 2
};

enum SetupType {
  SETUP_PASSWORD = 0,
  SETUP_FINGERPRINT = 1,
  SETUP_RFID = 2
};

Mode currentMode = MODE_NORMAL;
AuthType selectedAuthType = AUTH_KEYPAD;
SetupType selectedSetupType = SETUP_PASSWORD;
uint8_t menuPosition = 0; // Theo dõi vị trí menu (0=Keypad, 1=RFID, 2=Fingerprint)
uint8_t setupMenuPosition = 0; // Theo dõi vị trí menu setup
String enteredPassword = "";
String newPassword = "";
String systemPassword = DEFAULT_PASSWORD;
const int MAX_PASSWORD_LENGTH = 6;

// Biến lưu trữ dữ liệu cảm biến
struct SensorData {
  float temperature;
  float humidity;
  uint32_t gas;
  uint32_t light;
  unsigned long timestamp;
};

SensorData currentSensorData = {0, 0, 0, 0, 0};
SensorData lastSentData = {-999, -999, 9999999, 9999999, 0}; // Giá trị khởi tạo để force gửi lần đầu
String sensorBuffer = ""; // Buffer để nhận dữ liệu từ STM32

unsigned long lastSensorUpdate = 0;
const unsigned long SENSOR_UPDATE_INTERVAL = 5000; // Gửi tối đa 5 giây/lần

void setup() {
  Serial.begin(115200);
  
  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting WiFi...");
  }
  Serial.println("WiFi Connected");
  
  // Khởi tạo NTP
  timeClient.begin();
  timeClient.update(); // Lấy thời gian ngay khi khởi động
  
  // Khởi tạo Firebase
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  if (Firebase.ready()) {
    Serial.println("Firebase Connected!");
    if (Firebase.getString(fbdo, PASSWORD_PATH)) {
      systemPassword = fbdo.stringData();
    } else {
      Firebase.setString(fbdo, PASSWORD_PATH, DEFAULT_PASSWORD);
    }
  }
  
  // Khởi tạo các module
  SPI.begin();
  rfid.PCD_Init();
  mySerial.begin(57600, SERIAL_8N1, RXD2, TXD2);
  mySerial2.begin(9600, SERIAL_8N1, 26, 25);
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found!");
  }
  
  Serial.println("=== SYSTEM READY ===");
  Serial.println("A - Auth | C - Sensor | D - Setup");
}

void sendToSTM32(char cmd) {
  mySerial2.write(cmd);
  Serial.print("-> STM32: ");
  Serial.println(cmd);
}

String getCardUID() {
  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uidString += "0";
    }
    uidString += String(rfid.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();
  return uidString;
}

bool verifyPassword(String password) {
  return (password == systemPassword);
}

bool verifyCardUID(String uid) {
  if (!Firebase.ready() || uid.length() == 0) return false;
  
  char cardPath[50];
  sprintf(cardPath, "cards/%s", uid.c_str());
  return Firebase.get(fbdo, cardPath);
}

bool verifyFingerprint() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return false;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return false;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return false;
  
  Serial.print("Found fingerprint ID #");
  Serial.println(finger.fingerID);
  
  char fingerPath[50];
  sprintf(fingerPath, "fingerprints/%d", finger.fingerID);
  return Firebase.get(fbdo, fingerPath);
}

bool setupNewPassword(String password) {
  if (password.length() < 4 || password.length() > 6) {
    Serial.println("Password must be 4-6 digits!");
    return false;
  }
  
  if (Firebase.ready()) {
    if (Firebase.setString(fbdo, PASSWORD_PATH, password)) {
      systemPassword = password;
      Serial.println("Password updated!");
      return true;
    }
  }
  Serial.println("Password update failed!");
  return false;
}

bool setupNewFingerprint() {
  Serial.println("Starting fingerprint enrollment...");
  
  uint8_t id = 1;
  while (id < 127) {
    if (finger.loadModel(id) != FINGERPRINT_OK) {
      break;
    }
    id++;
  }
  
  if (id >= 127) {
    Serial.println("Fingerprint storage full!");
    return false;
  }
  
  Serial.print("Enrolling fingerprint ID #");
  Serial.println(id);
  
  uint8_t p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      Serial.println("Image captured!");
    } else if (p == FINGERPRINT_NOFINGER) {
      continue;
    } else {
      Serial.println("Image capture failed!");
      return false;
    }
  }
  
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Image convert failed!");
    return false;
  }
  
  Serial.println("Please place same finger again...");
  delay(2000);
  
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      Serial.println("Second image captured!");
    } else if (p == FINGERPRINT_NOFINGER) {
      continue;
    }
  }
  
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Second image convert failed!");
    return false;
  }
  
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Model creation failed!");
    return false;
  }
  
  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    Serial.println("Model storage failed!");
    return false;
  }
  
  if (Firebase.ready()) {
    char fingerPath[50];
    sprintf(fingerPath, "fingerprints/%d", id);
    if (Firebase.setBool(fbdo, fingerPath, true)) {
      Serial.println("Fingerprint enrolled successfully!");
      return true;
    }
  }
  
  Serial.println("Firebase storage failed!");
  return false;
}

bool setupNewRFIDCard(String uid) {
  if (uid.length() == 0) {
    Serial.println("Invalid card UID!");
    return false;
  }
  
  if (Firebase.ready()) {
    char cardPath[50];
    sprintf(cardPath, "cards/%s", uid.c_str());
    if (Firebase.setBool(fbdo, cardPath, true)) {
      Serial.println("RFID card enrolled successfully!");
      return true;
    }
  }
  
  Serial.println("RFID enrollment failed!");
  return false;
}

// Hàm kiểm tra thay đổi đáng kể của dữ liệu cảm biến
bool hasSignificantChange(SensorData current, SensorData &last) {
  return (fabs(current.temperature - last.temperature) >= TEMP_THRESHOLD ||
          fabs(current.humidity - last.humidity) >= HUMIDITY_THRESHOLD ||
          abs((int)current.gas - (int)last.gas) >= GAS_THRESHOLD ||
          abs((int)current.light - (int)last.light) >= LIGHT_THRESHOLD);
}

// Hàm gửi dữ liệu cảm biến lên Firebase
// Hàm gửi dữ liệu cảm biến lên Firebase (đã chỉnh sửa)
bool sendSensorDataToFirebase(SensorData &data) {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready!");
    return false;
  }
  Firebase.getInt(fbdo, "RL/RL1", &rl1);
  Firebase.getInt(fbdo, "RL/RL2", &rl2);
  Firebase.getInt(fbdo, "RL/RL3", &rl3);
  Firebase.getInt(fbdo, "RL/RL4", &rl4);
  if (rl1 == 0) {
    mySerial2.print("k");
  } else {
    mySerial2.print("j");
  }
  if (rl2 == 0) {
    mySerial2.print("h");
  } else {
    mySerial2.print("g");
  }
  if (rl3 == 0) {
    mySerial2.print("f");
  } else {
    mySerial2.print("d");
  }
  if (rl4 == 0) {
    mySerial2.print("s");
  } else {
    mySerial2.print("a");
  }

  // Lưu dữ liệu hiện tại vào sensors/current
  FirebaseJson sensorJson;
  sensorJson.set("temperature", data.temperature);
  sensorJson.set("humidity", data.humidity);
  sensorJson.set("gas", (int)data.gas);
  sensorJson.set("light", (int)data.light);
  unsigned long timestamp = timeClient.getEpochTime();
  sensorJson.set("timestamp", timestamp);

  String path = String(SENSOR_PATH) + "/current";
  bool success = Firebase.setJSON(fbdo, path, sensorJson);

  // Lưu lịch sử vào sensors/history
  String historyPath = String(SENSOR_PATH) + "/history/" + String(timestamp);
  Firebase.setJSON(fbdo, historyPath, sensorJson);

  // Kiểm tra và lưu cảnh báo nếu khí gas vượt ngưỡng
  if ((int)data.gas >= 3000) {
    FirebaseJson alertJson;
    alertJson.set("type", "Khí gas vượt ngưỡng");
    alertJson.set("severity", "Cao");
    alertJson.set("action", "Gửi thông báo");
    alertJson.set("timestamp", timestamp);
    String alertPath = "alerts/history/" + String(timestamp);
    Firebase.setJSON(fbdo, alertPath, alertJson);
    Serial.println("Alert: Gas level exceeded threshold!");
  }

  if (success) {
    Serial.println("Sensor data sent to Firebase:");
    Serial.printf("  Temperature: %.1f°C\n", data.temperature);
    Serial.printf("  Humidity: %.1f%%\n", data.humidity);
    Serial.printf("  Gas: %u\n", data.gas);
    Serial.printf("  Light: %u lux\n", data.light);
    Serial.printf("  Timestamp: %lu (UTC)\n", timestamp);
    return true;
  } else {
    Serial.println("Sensor data send failed!");
    Serial.println(fbdo.errorReason());
    return false;
  }
}

// Hàm phân tích dữ liệu cảm biến từ STM32
bool parseSensorData(String data, SensorData &sensorData) {
  if (!data.startsWith("SENSOR:")) {
    Serial.println("Invalid data format: " + data);
    return false;
  }
  
  String values = data.substring(7);
  Serial.println("Received sensor data: " + values);
  
  int commaIndex1 = values.indexOf(',');
  int commaIndex2 = values.indexOf(',', commaIndex1 + 1);
  int commaIndex3 = values.indexOf(',', commaIndex2 + 1);
  
  if (commaIndex1 == -1 || commaIndex2 == -1 || commaIndex3 == -1) {
    Serial.println("Data format error - missing commas");
    return false;
  }
  
  String tempStr = values.substring(0, commaIndex1);
  String humStr = values.substring(commaIndex1 + 1, commaIndex2);
  String gasStr = values.substring(commaIndex2 + 1, commaIndex3);
  String lightStr = values.substring(commaIndex3 + 1);
  
  sensorData.temperature = tempStr.toFloat();
  sensorData.humidity = humStr.toFloat();
  sensorData.gas = gasStr.toInt();
  sensorData.light = lightStr.toInt();
  sensorData.timestamp = millis(); // Chỉ dùng tạm, sẽ được cập nhật bởi NTP
  
  Serial.printf("Parsed - Temp: %.2f°C, Hum: %.2f%%, Gas: %u, Light: %u\n", 
                sensorData.temperature, sensorData.humidity, sensorData.gas, sensorData.light);
  
  return true;
}

// Hàm xử lý dữ liệu nhận từ STM32
void processSensorData() {
  unsigned long currentTime = millis();
  
  if (hasSignificantChange(currentSensorData, lastSentData) && 
      (currentTime - lastSensorUpdate) >= SENSOR_UPDATE_INTERVAL) {
    
    if (sendSensorDataToFirebase(currentSensorData)) {
      lastSentData = currentSensorData;
      lastSensorUpdate = currentTime;
    }
  }
}

void loop() {
  char key = keypad.getKey();
  
  // Cập nhật thời gian NTP
  timeClient.update();
  
  // Xử lý dữ liệu cảm biến từ STM32
  while (mySerial2.available()) {
    char inChar = mySerial2.read();
    if (inChar == '\n') {
      // Kết thúc một dòng dữ liệu
      if (parseSensorData(sensorBuffer, currentSensorData)) {
        processSensorData(); // Kiểm tra và gửi lên Firebase nếu cần
      }
      sensorBuffer = ""; // Reset buffer
    } else if (inChar != '\r') {
      sensorBuffer += inChar; // Thêm ký tự vào buffer (bỏ qua \r)
    }
  }
  
  if (key) {
    Serial.print("Key: ");
    Serial.println(key);
    
    switch (currentMode) {
      case MODE_NORMAL:
        if (key == 'A') {
          sendToSTM32('A'); // Gửi A để STM32 hiển thị menu xác thực
          menuPosition = 0; // Reset về vị trí đầu
          Serial.println("Auth mode - Press B to select, # to confirm");
        }
        else if (key == 'C') {
          sendToSTM32('C'); // Gửi C để STM32 hiển thị cảm biến
          Serial.println("Sensor display mode");
        }
        else if (key == 'D') {
          sendToSTM32('W'); // Gửi W để STM32 hiển thị màn hình xác thực setup
          Serial.println("Setup authentication required");
          currentMode = MODE_VERIFY_SETUP;
          enteredPassword = "";
        }
        else if (key == 'B') {
          sendToSTM32('B'); // Di chuyển con trỏ menu
          menuPosition = (menuPosition + 1) % 3; // 0->1->2->0
          Serial.print("Selected: ");
          switch(menuPosition) {
            case 0: Serial.println("KEYPAD"); break;
            case 1: Serial.println("RFID"); break;
            case 2: Serial.println("FINGERPRINT"); break;
          }
        }
        else if (key == '#') {
          sendToSTM32('#'); // Xác nhận lựa chọn
          Serial.println("Confirmed - Waiting for STM32");
          delay(500); // Đợi STM32 xử lý
          
          // Gửi lệnh tương ứng với loại xác thực được chọn
          switch(menuPosition) {
            case 0: // Keypad
              sendToSTM32('P'); // Báo bắt đầu nhập mật khẩu
              Serial.println("Password input mode");
              currentMode = MODE_AUTH_PASSWORD;
              break;
            case 1: // RFID
              sendToSTM32('R'); // Báo bắt đầu xác thực RFID
              Serial.println("RFID auth mode");
              currentMode = MODE_AUTH_RFID;
              break;
            case 2: // Fingerprint
              sendToSTM32('V'); // Báo bắt đầu xác thực vân tay
              Serial.println("Fingerprint auth mode");
              currentMode = MODE_AUTH_FINGERPRINT;
              break;
          }
          enteredPassword = "";
        }
        break;
        
      case MODE_VERIFY_SETUP:
        if (key >= '0' && key <= '9') {
          if (enteredPassword.length() < MAX_PASSWORD_LENGTH) {
            enteredPassword += key;
            sendToSTM32(key); // Gửi số cho STM32 hiển thị
            Serial.print("*");
          }
        }
        else if (key == '#') {
          Serial.println();
          Serial.println("Verifying setup password...");
          if (verifyPassword(enteredPassword)) {
            sendToSTM32('S'); // Gửi S để STM32 hiển thị menu setup
            setupMenuPosition = 0; // Reset về vị trí đầu
            currentMode = MODE_SETUP;
            Serial.println("Password correct - Setup mode");
            Serial.println("Press B to select, # to confirm");
          } else {
            sendToSTM32('F'); // Sai mật khẩu
            Serial.println("Wrong password! Cannot enter setup");
            currentMode = MODE_NORMAL;
          }
          enteredPassword = "";
        }
        else if (key == '*') {
          if (enteredPassword.length() > 0) {
            enteredPassword = enteredPassword.substring(0, enteredPassword.length() - 1);
            sendToSTM32('X'); // Xóa ký tự cuối
            Serial.print("\b \b"); // Xóa ký tự trên Serial
          } else {
            enteredPassword = "";
            currentMode = MODE_NORMAL;
            Serial.println("Setup auth cancelled");
          }
        }
        break;
        
      case MODE_SETUP:
        if (key == 'B') {
          sendToSTM32('B'); // Di chuyển con trỏ menu setup
          setupMenuPosition = (setupMenuPosition + 1) % 3; // 0->1->2->0
          Serial.print("Setup: ");
          switch(setupMenuPosition) {
            case 0: Serial.println("NEW PASSWORD"); break;
            case 1: Serial.println("NEW FINGERPRINT"); break;
            case 2: Serial.println("NEW RFID CARD"); break;
          }
        }
        else if (key == '#') {
          sendToSTM32('#'); // Xác nhận lựa chọn setup
          delay(500);
          
          switch(setupMenuPosition) {
            case 0: // Setup mật khẩu
              sendToSTM32('N');
              Serial.println("New password setup mode");
              currentMode = MODE_SETUP_PASSWORD;
              newPassword = "";
              break;
            case 1: // Setup vân tay
              sendToSTM32('G');
              Serial.println("New fingerprint setup mode");
              currentMode = MODE_SETUP_FINGERPRINT;
              break;
            case 2: // Setup RFID
              sendToSTM32('T');
              Serial.println("New RFID card setup mode");
              currentMode = MODE_SETUP_RFID;
              break;
          }
        }
        else if (key == '*') {
          currentMode = MODE_NORMAL;
          Serial.println("Setup cancelled - Back to main menu");
        }
        break;
        
      case MODE_SETUP_PASSWORD:
        if (key >= '0' && key <= '9') {
          if (newPassword.length() < MAX_PASSWORD_LENGTH) {
            newPassword += key;
            sendToSTM32(key);
            Serial.print("*");
          }
        }
        else if (key == '#') {
          Serial.println();
          if (setupNewPassword(newPassword)) {
            sendToSTM32('Y'); // Setup thành công
            Serial.println("New password set!");
            delay(3000); // Hiển thị thông báo 3 giây
            sendToSTM32('I'); // Quay về intro
            Serial.println("Back to main screen");
          } else {
            sendToSTM32('E'); // Lỗi setup
            Serial.println("Password setup failed!");
            delay(3000);
            sendToSTM32('I'); // Quay về intro
          }
          newPassword = "";
          currentMode = MODE_NORMAL;
        }
        else if (key == '*') {
          if (newPassword.length() > 0) {
            newPassword = newPassword.substring(0, newPassword.length() - 1);
            sendToSTM32('X');
            Serial.print("\b \b");
          } else {
            currentMode = MODE_NORMAL;
            Serial.println("Password setup cancelled");
          }
        }
        break;
        
      case MODE_SETUP_FINGERPRINT:
        if (key == '#') {
          Serial.println("Starting fingerprint enrollment...");
          if (setupNewFingerprint()) {
            sendToSTM32('Y'); // Setup thành công
            Serial.println("New fingerprint enrolled!");
            delay(3000); // Hiển thị thông báo 3 giây
            sendToSTM32('I'); // Quay về intro
            Serial.println("Back to main screen");
          } else {
            sendToSTM32('E'); // Lỗi setup
            Serial.println("Fingerprint enrollment failed!");
            delay(3000);
            sendToSTM32('I'); // Quay về intro
          }
          currentMode = MODE_NORMAL;
        }
        else if (key == '*') {
          currentMode = MODE_NORMAL;
          sendToSTM32('I'); // Quay về intro khi hủy
          Serial.println("Fingerprint setup cancelled");
        }
        break;
        
      case MODE_SETUP_RFID:
        if (key == '*') {
          currentMode = MODE_NORMAL;
          sendToSTM32('I'); // Quay về intro khi hủy
          Serial.println("RFID setup cancelled");
        }
        break;
        
      case MODE_AUTH_PASSWORD:
        if (key >= '0' && key <= '9') {
          if (enteredPassword.length() < MAX_PASSWORD_LENGTH) {
            enteredPassword += key;
            sendToSTM32(key); // Gửi số cho STM32 hiển thị
            Serial.print("*");
          }
        }
        else if (key == '#') {
          Serial.println();
          Serial.println("Verifying password...");
          if (verifyPassword(enteredPassword)) {
            sendToSTM32('O'); // Xác thực thành công
            Serial.println("Password correct - Door open!");
          } else {
            sendToSTM32('F'); // Sai mật khẩu
            Serial.println("Wrong password!");
          }
          enteredPassword = "";
          Serial.println("Back to NORMAL mode");
          currentMode = MODE_NORMAL;
        }
        else if (key == '*') {
          if (enteredPassword.length() > 0) {
            enteredPassword = enteredPassword.substring(0, enteredPassword.length() - 1);
            sendToSTM32('X'); // Xóa ký tự cuối
            Serial.print("\b \b"); // Xóa ký tự trên Serial
          } else {
            enteredPassword = "";
            currentMode = MODE_NORMAL;
            sendToSTM32('I'); // Quay về intro khi hủy
            Serial.println("Password input cancelled");
          }
        }
        break;
        
      case MODE_AUTH_RFID:
        if (key == '*') {
          currentMode = MODE_NORMAL;
          sendToSTM32('I'); // Quay về intro khi hủy
          Serial.println("RFID auth cancelled - Back to main menu");
        }
        break;
        
      case MODE_AUTH_FINGERPRINT:
        if (key == '*') {
          currentMode = MODE_NORMAL;
          sendToSTM32('I'); // Quay về intro khi hủy
          Serial.println("Fingerprint auth cancelled - Back to main menu");
        }
        break;
    }
  }
  
  // Xử lý RFID cho xác thực
  if (currentMode == MODE_AUTH_RFID && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = getCardUID();
    Serial.print("RFID: ");
    Serial.println(uid);
    
    if (verifyCardUID(uid)) {
      sendToSTM32('O'); // Xác thực thành công
      Serial.println("Valid card - Door open!");
    } else {
      sendToSTM32('U'); // Xác thực RFID thất bại
      Serial.println("Invalid card!");
    }
    
    currentMode = MODE_NORMAL;
    rfid.PICC_HaltA();
  }
  
  // Xử lý RFID cho setup thẻ mới
  if (currentMode == MODE_SETUP_RFID && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = getCardUID();
    Serial.print("Setup RFID: ");
    Serial.println(uid);
    
    if (setupNewRFIDCard(uid)) {
      sendToSTM32('Y'); // Setup thành công
      Serial.println("New RFID card enrolled!");
      delay(3000); // Hiển thị thông báo 3 giây
      sendToSTM32('I'); // Quay về intro
      Serial.println("Back to main screen");
    } else {
      sendToSTM32('E'); // Lỗi setup
      Serial.println("RFID enrollment failed!");
      delay(3000);
      sendToSTM32('I'); // Quay về intro
    }
    
    currentMode = MODE_NORMAL;
    rfid.PICC_HaltA();
  }
  
  if (currentMode == MODE_AUTH_FINGERPRINT) {
    if (verifyFingerprint()) {
      sendToSTM32('O'); // Xác thực thành công
      Serial.println("Valid fingerprint - Door open!");
      currentMode = MODE_NORMAL;
    } else {
      uint8_t p = finger.getImage();
      if (p == FINGERPRINT_OK) {
        // Có ảnh nhưng không khớp
        sendToSTM32('Z'); // Xác thực vân tay thất bại
        Serial.println("Invalid fingerprint!");
        currentMode = MODE_NORMAL;
      }
      // Nếu chưa có ảnh thì tiếp tục chờ
    }
  }
}
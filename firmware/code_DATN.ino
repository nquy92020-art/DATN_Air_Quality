#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID           "TMPL6HHDOGsjx"
#define BLYNK_TEMPLATE_NAME         "DATN"
char BLYNK_AUTH_TOKEN[64]   =   "";
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "index_html.h"
#include "data_config.h"
#include <EEPROM.h>
#include <Arduino_JSON.h>
#include "icon.h"
#define BUTTON1_PIN     33
#define BUTTON2_PIN     32
#define BUTTON3_PIN     35
#define BUTTON4_PIN     34
#define BUTTON5_PIN     39
struct ButtonControl {
  int pin;
  bool lastButtonState;
  bool buttonState;
  bool relayState;
  bool manualControl;
  unsigned long lastDebounceTime;
  unsigned long lastToggleTime;
  int toggleCount;
  unsigned long pressStartTime;
  bool longPressHandled;
  bool longPressTriggered;  // Thêm flag để đánh dấu đã xử lý long press
};
// Tạo mảng quản lý 5 nút nhấn
ButtonControl buttons[5] = {
  {BUTTON1_PIN, HIGH, HIGH, false, false, 0, 0, 0, 0, false},
  {BUTTON2_PIN, HIGH, HIGH, false, false, 0, 0, 0, 0, false},
  {BUTTON3_PIN, HIGH, HIGH, false, false, 0, 0, 0, 0, false},
  {BUTTON4_PIN, HIGH, HIGH, false, false, 0, 0, 0, 0, false},
  {BUTTON5_PIN, HIGH, HIGH, false, false, 0, 0, 0, 0, false}
};
// Khai báo nguyên mẫu các hàm điều khiển relay (để dùng trong mảng hàm)
void controlRelay1(bool state);
void controlRelay2(bool state);
void controlRelay3(bool state);
void controlRelay4(bool state);
void controlRelay5(bool state);
// Mảng chứa các hàm điều khiển relay tương ứng
typedef void (*RelayControlFunc)(bool);
RelayControlFunc relayControlFuncs[5] = {
  controlRelay1,
  controlRelay2,
  controlRelay3,
  controlRelay4,
  controlRelay5
};
// Các biến cài đặt cho nút nhấn
const unsigned long debounceDelay = 50;
const unsigned long doubleClickDelay = 300; // Thời gian cho nhấn đúp 
const unsigned long longPressDelay = 10000; // 10 giây
// Khai báo hàm
void initButtons();
void handleButtons();
void toggleRelay(int index);
void checkAndControlRelays();
// Tạo đối tượng AsyncWebServer trên cổng 80
AsyncWebServer server(80);
bool apModeStarted = false;
volatile bool configSaveOK = false;
volatile bool restartRequested = false;
volatile unsigned long restartRequestAt = 0;
//----------------------- Khai báo 1 số biến Blynk -----------------------
bool blynkConnect = false;
volatile bool blynkOnline = false;
BlynkTimer timer; 

// Blynk chi duoc goi truc tiep trong TaskBlynk (Core 0).
// Cac task local gui event qua queue de tranh truy cap Blynk dong thoi tu 2 core.
struct BlynkEventMessage {
  char eventCode[24];
  char message[192];
};
QueueHandle_t blynkEventQueue = NULL;

void queueBlynkEvent(const char *eventCode, const String &message) {
  if (blynkEventQueue == NULL || !blynkOnline || message.length() == 0) return;

  BlynkEventMessage item = {};
  strncpy(item.eventCode, eventCode, sizeof(item.eventCode) - 1);
  strncpy(item.message, message.c_str(), sizeof(item.message) - 1);
  xQueueSend(blynkEventQueue, &item, 0);
}
// Một số Macro
#define ENABLE    1
#define DISABLE   0
// ---------------------- Khai báo cho OLED 1.3 --------------------------
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#define i2c_Address 0x3C // khởi tạo với địa chỉ I2C 0x3C Thường là OLED của eBay
#define SCREEN_WIDTH 128 // Chiều rộng màn hình OLED, tính bằng pixel
#define SCREEN_HEIGHT 64 // Chiều cao màn hình OLED, tính bằng pixel
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G oled = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define OLED_SDA      21
#define OLED_SCL      22
typedef enum {
  SCREEN0,
  SCREEN1,
  SCREEN2,
  SCREEN3,
  SCREEN4,
  SCREEN5,
  SCREEN6,
  SCREEN7,
  SCREEN8,
  SCREEN9,
  SCREEN10,
  SCREEN11
}SCREEN;
int screenOLED = SCREEN0;
bool enableShow = DISABLE;
#define SAD    0
#define NORMAL 1
#define HAPPY  2
int warningTempState = SAD;
int warningHumiState = NORMAL;
int warningDustState = HAPPY;
bool autoWarning = DISABLE;
// --------------------- Khai báo Cảm biến DHT11 ---------------------
#include "DHT.h"
#define DHT11_PIN         18
#define DHTTYPE DHT11
DHT dht(DHT11_PIN, DHTTYPE);
float tempValue = 30;
int humiValue   = 60;
bool dht11ReadOK = false;
// -------------------- Khai báo cảm biến bụi --------------
#define DUST_TRIG             23
#define DUST_ANALOG           36
int dustValue = 0;
TaskHandle_t TaskOLEDDisplay_handle = NULL;
TaskHandle_t TaskDHT11_handle = NULL;
TaskHandle_t TaskDustSensor_handle = NULL;
TaskHandle_t TaskAutoWarning_handle = NULL;
TaskHandle_t TaskRelayControl_handle = NULL;
TaskHandle_t TaskBlynk_handle = NULL;
TaskHandle_t TaskNetwork_handle = NULL;

// Trạng thái kết nối dùng chung cho OLED và các task
volatile bool wifiConnected = false;
// ---------------------- Khai báo chân Relay --------------------------
#define RELAY1_PIN     25
#define RELAY2_PIN     26
#define RELAY3_PIN     27
#define RELAY4_PIN     14
#define RELAY5_PIN     13   
// ---------------------- Biến trạng thái Relay ------------------------
bool relay1State = false;
bool relay2State = false;
bool relay3State = false;
bool relay4State = false;
bool relay5State = false;

// Vùng trễ Hysteresis - sẽ tinh chỉnh sau khi thử nghiệm thực tế
const float TEMP_HYSTERESIS = 2.0f;  // °C
const int HUMI_HYSTERESIS = 5;       // %RH
const int DUST_HYSTERESIS = 10;      // ug/m3
// Khai báo hàm điều khiển relay
void controlRelay1(bool state);
void controlRelay2(bool state);
void controlRelay3(bool state);
void controlRelay4(bool state);
void controlRelay5(bool state);
void turnOffAllRelays();
// ---------------------- Biến trạng thái Nút nhấn ------------------------
bool button1State = false;
bool button2State = false;
bool button3State = false;
bool button4State = false;
bool button5State = false;
void setup(){
  vTaskDelay(pdMS_TO_TICKS(1000)); // Chờ nguồn ổn định
  Serial.begin(115200); 
  // Đọc data setup từ eeprom
  EEPROM.begin(512);
  readEEPROM();

  blynkEventQueue = xQueueCreate(6, sizeof(BlynkEventMessage));
  // Khởi tạo OLED
  oled.begin(i2c_Address, true);
  oled.setTextSize(2);
  oled.setTextColor(SH110X_WHITE);
  delay(100);
  // Khởi tạo DHT11
  dht.begin();
  // Khoi tao cam bien bui theo timing cua GP2Y1010AU0F
  pinMode(DUST_TRIG, OUTPUT);
  digitalWrite(DUST_TRIG, HIGH);   // LED hong ngoai tat khi nghi
  pinMode(DUST_ANALOG, INPUT);
  analogReadResolution(12);
  // Khởi tạo Relay
  initRelays();  
  // Khởi tạo Nút nhấn
  initButtons(); 
  // Tạo nhiệm vụ
  xTaskCreatePinnedToCore(TaskOLEDDisplay,     "TaskOLEDDisplay" ,     1024*16 ,  NULL,  20 ,  &TaskOLEDDisplay_handle  , 1);
  xTaskCreatePinnedToCore(TaskDHT11,           "TaskDHT11" ,           1024*10 ,  NULL,  10 ,  &TaskDHT11_handle  , 1);
  xTaskCreatePinnedToCore(TaskDustSensor,      "TaskDustSensor" ,      1024*16 ,  NULL,  10 ,  &TaskDustSensor_handle  , 1);
  xTaskCreatePinnedToCore(TaskAutoWarning,     "TaskAutoWarning" ,     1024*10 ,  NULL,  10 ,  &TaskAutoWarning_handle ,  1);
  xTaskCreatePinnedToCore(TaskRelayControl,    "TaskRelayControl",     1024*16,   NULL,  15 ,  &TaskRelayControl_handle, 1);
  // Kết nối mạng chạy ở task riêng để không chặn cảm biến/OLED/AUTO
  xTaskCreatePinnedToCore(TaskNetwork,         "TaskNetwork",          1024*16,   NULL,   8 ,  &TaskNetwork_handle,      0);
}
void loop() {
  vTaskDelete(NULL);
}
//--------------------Task đo DHT11 ---------------
void TaskDHT11(void *pvParameters) { 
    while(1) {
      int humi =  dht.readHumidity();
      float temp =  dht.readTemperature();
      if (isnan(humi) || isnan(temp) ) {
          Serial.println(F("Sensor Error!"));
          dht11ReadOK = false;
      }
      else if(humi <= 100 && temp < 100) {
          dht11ReadOK = true;
          humiValue = humi;
          tempValue = temp;
          Serial.print(F("Humidity: "));
          Serial.print(humiValue);
          Serial.print(F("%  Temperature: "));
          Serial.print(tempValue);
          Serial.print(F("°C "));
          Serial.println();
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
volatile int dustADC = 0;
volatile bool dustReady = false;
void TaskDustSensor(void *pvParameters) {

  float adcFiltered = 0.0f;
  bool firstSample = true;

  // Bo dem 5 mau ADC gan nhat de loc trung vi (median)
  int adcBuffer[5] = {0};
  int adcIndex = 0;
  int adcCount = 0;

  // Chi cap nhat gia tri PM2.5 cong khai moi 1 giay
  unsigned long lastDustUpdate = 0;

  while (1) {

    // Bat LED hong ngoai cua cam bien
    digitalWrite(DUST_TRIG, LOW);
    delayMicroseconds(280);

    // Doc ADC dung tai thoi diem lay mau
    int rawADC = analogRead(DUST_ANALOG);
    dustADC = rawADC;

    delayMicroseconds(40);

    // Tat LED va hoan tat chu ky ~10 ms
    digitalWrite(DUST_TRIG, HIGH);
    delayMicroseconds(9680);

    // ---------------------------------------------------------
    // Lop 1: Median 5 mau gan nhat de loai cac xung nhieu dot bien
    // ---------------------------------------------------------
    adcBuffer[adcIndex] = rawADC;
    adcIndex = (adcIndex + 1) % 5;

    if (adcCount < 5) {
      adcCount++;
    }

    int medianADC = rawADC;

    if (adcCount == 5) {
      int temp[5];

      for (int i = 0; i < 5; i++) {
        temp[i] = adcBuffer[i];
      }

      // Sap xep 5 phan tu de lay phan tu o giua
      for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 5; j++) {
          if (temp[j] < temp[i]) {
            int t = temp[i];
            temp[i] = temp[j];
            temp[j] = t;
          }
        }
      }

      medianADC = temp[2];
    }

    // ---------------------------------------------------------
    // Lop 2: EMA 90% gia tri cu + 10% gia tri median moi
    // ---------------------------------------------------------
    if (firstSample) {
      adcFiltered = medianADC;
      firstSample = false;
    } else {
      adcFiltered = 0.9f * adcFiltered + 0.1f * medianADC;
    }

    const float ADC_REFERENCE = 2385.0f;
    const float PM25_REFERENCE = 15.0f;
    const float PM25_SCALE = 0.35f;

    float dustDensity =
        PM25_REFERENCE +
        (adcFiltered - ADC_REFERENCE) * PM25_SCALE;
    // Gioi han de tranh gia tri am / dot bien vo ly
    dustDensity = constrain(dustDensity, 3.0f, 300.0f);

    int newDustValue = (int)(dustDensity + 0.5f);

    // ---------------------------------------------------------
    // Lop 3: Chi cap nhat PM2.5 moi 1 giay de OLED/Blynk on dinh
    // ---------------------------------------------------------
    if (lastDustUpdate == 0 || millis() - lastDustUpdate >= 1000) {
      dustValue = newDustValue;
      lastDustUpdate = millis();
    }

    // Da co du lieu ADC de he thong tiep tuc xu ly
    dustReady = true;

    Serial.print("ADC raw: ");
    Serial.print(rawADC);
    Serial.print(" | ADC median: ");
    Serial.print(medianADC);
    Serial.print(" | ADC filtered: ");
    Serial.print((int)adcFiltered);
    Serial.print(" | PM2.5: ");
    Serial.print(dustValue);
    Serial.println(" ug/m3");

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ---------------------- Hàm khởi tạo Relay ---------------------------
void initRelays() {
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  pinMode(RELAY5_PIN, OUTPUT);
  turnOffAllRelays();
}
// ---------------------- Hàm điều khiển Relay -------------------------
void turnOffAllRelays() {
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(RELAY4_PIN, HIGH);
  digitalWrite(RELAY5_PIN, HIGH);
  relay1State = false;
  relay2State = false;
  relay3State = false;
  relay4State = false;
  relay5State = false;
  // Đồng bộ với mảng buttons
  for(int i = 0; i < 5; i++) {
    buttons[i].relayState = false;
  }
}
void controlRelay1(bool state) {
  digitalWrite(RELAY1_PIN, !state);
  relay1State = state;
}
void controlRelay2(bool state) {
  digitalWrite(RELAY2_PIN, !state);
  relay2State = state;
}
void controlRelay3(bool state) {
  digitalWrite(RELAY3_PIN, !state);
  relay3State = state;
}
void controlRelay4(bool state) {
  digitalWrite(RELAY4_PIN, !state);
  relay4State = state;
}
void controlRelay5(bool state) {
  digitalWrite(RELAY5_PIN, !state);
  relay5State = state;
}
// ==================== CÁC HÀM XỬ LÝ NÚT NHẤN ====================
void initButtons() {
  for (int i = 0; i < 5; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
    // Đồng bộ trạng thái ban đầu với relay
    switch(i) {
      case 0: buttons[i].relayState = relay1State; break;
      case 1: buttons[i].relayState = relay2State; break;
      case 2: buttons[i].relayState = relay3State; break;
      case 3: buttons[i].relayState = relay4State; break;
      case 4: buttons[i].relayState = relay5State; break;
    }
    // Khởi tạo các flag mới
    buttons[i].longPressHandled = false;
    buttons[i].longPressTriggered = false;
    buttons[i].pressStartTime = 0;
  }
}
void toggleRelay(int index) {
  // Đảo trạng thái relay
  buttons[index].relayState = !buttons[index].relayState;
  // Gọi hàm điều khiển relay tương ứng
  relayControlFuncs[index](buttons[index].relayState);
  // Đặt cờ điều khiển thủ công
  buttons[index].manualControl = true;
  // Cập nhật thời gian và số lần chuyển đổi
  buttons[index].lastToggleTime = millis();
  buttons[index].toggleCount++;
  // Đồng bộ với các biến toàn cục
  switch(index) {
    case 0: button1State = buttons[index].relayState; break;
    case 1: button2State = buttons[index].relayState; break;
    case 2: button3State = buttons[index].relayState; break;
    case 3: button4State = buttons[index].relayState; break;
    case 4: button5State = buttons[index].relayState; break;
  }
}
void handleButtons() {
  unsigned long currentTime = millis();

  for (int i = 0; i < 5; i++) {
    bool reading = digitalRead(buttons[i].pin);

    // Bat dau dem debounce moi khi muc logic thay doi.
    if (reading != buttons[i].lastButtonState) {
      buttons[i].lastDebounceTime = currentTime;
      buttons[i].lastButtonState = reading;
    }

    // Chi chap nhan trang thai sau khi on dinh du debounceDelay.
    if ((currentTime - buttons[i].lastDebounceTime) >= debounceDelay &&
        reading != buttons[i].buttonState) {
      buttons[i].buttonState = reading;

      if (buttons[i].buttonState == LOW) {
        // Canh nhan xuong da duoc debounce.
        buttons[i].pressStartTime = currentTime;
        buttons[i].longPressHandled = false;
        buttons[i].longPressTriggered = false;
      } else {
        // Canh nha nut da duoc debounce.
        if (buttons[i].pressStartTime > 0 &&
            !buttons[i].longPressHandled &&
            !buttons[i].longPressTriggered) {
          unsigned long pressDuration = currentTime - buttons[i].pressStartTime;
          if (pressDuration < longPressDelay &&
              (currentTime - buttons[i].lastToggleTime) >= doubleClickDelay) {
            toggleRelay(i);
          }
        }

        buttons[i].pressStartTime = 0;
        buttons[i].longPressHandled = false;
        buttons[i].longPressTriggered = false;
      }
    }

    // Nhan giu 10 giay nut 1 -> vao AP mode cau hinh.
    if (i == 0 &&
        buttons[i].buttonState == LOW &&
        buttons[i].pressStartTime > 0 &&
        !buttons[i].longPressTriggered &&
        (currentTime - buttons[i].pressStartTime) >= longPressDelay) {
      buttons[i].longPressHandled = true;
      buttons[i].longPressTriggered = true;

      Serial.println("Long press Button 1 -> AP mode");
      enableShow = ENABLE;
      screenOLED = SCREEN1;

      WiFi.disconnect(true);
      vTaskDelay(pdMS_TO_TICKS(200));
      connectAPMode();
      return;
    }
  }
}
// ==================== ĐIỀU KHIỂN TỰ ĐỘNG THEO CẢM BIẾN ====================
void checkAndControlRelays() {
  // autoWarning = 1: AUTO. autoWarning = 0: MANUAL.
  if (autoWarning != 1) return;

  // Hàm cục bộ: tải bật khi giá trị vượt ngưỡng CAO
  auto highHysteresis = [](float value, float onTh, float offTh, bool currentState) -> bool {
    if (!currentState && value >= onTh) return true;
    if ( currentState && value <= offTh) return false;
    return currentState;
  };

  // Hàm cục bộ: tải bật khi giá trị xuống dưới ngưỡng THẤP
  auto lowHysteresis = [](float value, float onTh, float offTh, bool currentState) -> bool {
    if (!currentState && value <= onTh) return true;
    if ( currentState && value >= offTh) return false;
    return currentState;
  };

  // Relay 1: máy lọc không khí - PM2.5 cao
  // dustReady chi dam bao da co mau dau tien sau khi khoi dong.
  if (!buttons[0].manualControl) {
    if (!dustReady) {
      if (buttons[0].relayState) {
        buttons[0].relayState = false;
        controlRelay1(false);
      }
    } else {
      float onTh  = EdustThreshold2;
      float offTh = max(0.0f, (float)EdustThreshold2 - DUST_HYSTERESIS);
      bool nextState = highHysteresis(dustValue, onTh, offTh, buttons[0].relayState);
      if (nextState != buttons[0].relayState) {
        buttons[0].relayState = nextState;
        controlRelay1(nextState);
      }
    }
  }
  // Bao ve khi DHT11 doc loi
  if (!dht11ReadOK) {

  if (!buttons[1].manualControl) {
    buttons[1].relayState = false;
    controlRelay2(false);
  }

  if (!buttons[2].manualControl) {
    buttons[2].relayState = false;
    controlRelay3(false);
  }

  if (!buttons[3].manualControl) {
    buttons[3].relayState = false;
    controlRelay4(false);
  }

  if (!buttons[4].manualControl) {
    buttons[4].relayState = false;
    controlRelay5(false);
  }

    return;
  }

  // Relay 2: quạt/làm mát - nhiệt độ cao
  if (!buttons[1].manualControl) {
    float onTh  = EtempThreshold2;
    float offTh = EtempThreshold2 - TEMP_HYSTERESIS;
    bool nextState = highHysteresis(tempValue, onTh, offTh, buttons[1].relayState);
    if (nextState != buttons[1].relayState) {
      buttons[1].relayState = nextState;
      controlRelay2(nextState);
    }
  }

  // Relay 3: sưởi - nhiệt độ thấp
  if (!buttons[2].manualControl) {
    float onTh  = EtempThreshold1;
    float offTh = EtempThreshold1 + TEMP_HYSTERESIS;
    bool nextState = lowHysteresis(tempValue, onTh, offTh, buttons[2].relayState);
    if (nextState != buttons[2].relayState) {
      buttons[2].relayState = nextState;
      controlRelay3(nextState);
    }
  }

  // Relay 4: hút ẩm - độ ẩm cao
  if (!buttons[3].manualControl) {
    float onTh  = EhumiThreshold2;
    float offTh = max(0.0f, (float)EhumiThreshold2 - HUMI_HYSTERESIS);
    bool nextState = highHysteresis(humiValue, onTh, offTh, buttons[3].relayState);
    if (nextState != buttons[3].relayState) {
      buttons[3].relayState = nextState;
      controlRelay4(nextState);
    }
  }

  // Relay 5: tạo ẩm - độ ẩm thấp
  if (!buttons[4].manualControl) {
    float onTh  = EhumiThreshold1;
    float offTh = EhumiThreshold1 + HUMI_HYSTERESIS;
    bool nextState = lowHysteresis(humiValue, onTh, offTh, buttons[4].relayState);
    if (nextState != buttons[4].relayState) {
      buttons[4].relayState = nextState;
      controlRelay5(nextState);
    }
  }

  relay1State = buttons[0].relayState;
  relay2State = buttons[1].relayState;
  relay3State = buttons[2].relayState;
  relay4State = buttons[3].relayState;
  relay5State = buttons[4].relayState;
}

// ==================== ĐIỀU KHIỂN TỪ BLYNK APP ====================
// Điều khiển Relay từ Blynk App
BLYNK_WRITE(V5) {
  int state = param.asInt();
  controlRelay1(state);
  buttons[0].manualControl = true;
  buttons[0].relayState = state;
  button1State = state;
}
BLYNK_WRITE(V6) {
  int state = param.asInt();
  controlRelay2(state);
  buttons[1].manualControl = true;
  buttons[1].relayState = state;
  button2State = state;
}
BLYNK_WRITE(V7) {
  int state = param.asInt();
  controlRelay3(state);
  buttons[2].manualControl = true;
  buttons[2].relayState = state;
  button3State = state;
}
BLYNK_WRITE(V8) {
  int state = param.asInt();
  controlRelay4(state);
  buttons[3].manualControl = true;
  buttons[3].relayState = state;
  button4State = state;
}
BLYNK_WRITE(V9) {
  int state = param.asInt();
  controlRelay5(state);
  buttons[4].manualControl = true;
  buttons[4].relayState = state;
  button5State = state;
}
// ==================== TASK ĐIỀU KHIỂN RELAY ====================
void TaskRelayControl(void *pvParameters) {
  while (1) {
    handleButtons();
    checkAndControlRelays();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
// Xóa 1 ô hình chữ nhật từ tọa độ (x1,y1) đến (x2,y2)
void clearRectangle(int x1, int y1, int x2, int y2) {
   for(int i = y1; i < y2; i++) {
     oled.drawLine(x1, i, x2, i, 0);
   }
}
void clearOLED(){
  oled.clearDisplay();
  oled.display();
}
int countSCREEN7 = 0;
// Task hiển thị OLED
void TaskOLEDDisplay(void *pvParameters) {
  while (1) {
      switch(screenOLED) {
        case SCREEN0: // Hiệu ứng khởi động
          for(int j = 0; j < 3; j++) {
            for(int i = 0; i < FRAME_COUNT_loadingOLED; i++) {
              oled.clearDisplay();
              oled.drawBitmap(32, 0, loadingOLED[i], FRAME_WIDTH_64, FRAME_HEIGHT_64, 1);
              oled.display();
              delay(FRAME_DELAY/4);
            }
          }
          screenOLED = SCREEN1;
          break;
        case SCREEN1:   // Màn hình chính: luôn hiển thị dữ liệu local
          oled.clearDisplay();
          oled.setTextColor(SH110X_WHITE);
          oled.setTextSize(1);

          // Nhiet do
          oled.setCursor(0, 0);
          oled.print("NHIET DO: ");
          oled.print(tempValue, 1);
          oled.print(" C");
          // Do am
          oled.setCursor(0, 12);
          oled.print("DO AM: ");
          oled.print(humiValue);
          oled.print(" %");
          // Bui PM2.5
          oled.setCursor(0, 24);
          oled.print("PM2.5: ");
          oled.print(dustValue);
          oled.print(" ug/m3");

          // Che do dieu khien
          oled.setCursor(0, 36);
          oled.print("CHE DO: ");
          oled.print(autoWarning == 1 ? "TU DONG" : "THU CONG");
          oled.setCursor(0, 50);

          if (apModeStarted) {
            oled.print("AP: 192.168.4.1");
          }
          else if (WiFi.status() == WL_CONNECTED) {
            oled.print("WiFi:OK ");

            if (Blynk.connected()) {
              oled.print("Blynk:OK");
            } else {
              oled.print("Blynk:--");
            }
          }
          else {
            oled.print("WiFi:-- Blynk:--");
          }

            oled.display();
            vTaskDelay(pdMS_TO_TICKS(250));
            break;
        case SCREEN2:    // Đang kết nối Wifi
          oled.clearDisplay();
          oled.setTextSize(1);
          oled.setCursor(40, 5);
          oled.print("WIFI");
          oled.setTextSize(1.5);
          oled.setCursor(40, 17);
          oled.print("Dang ket noi..");     
          for(int i = 0; i < FRAME_COUNT_wifiOLED; i++) {
            clearRectangle(0, 0, 32, 32);
            oled.drawBitmap(0, 0, wifiOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.display();
            delay(FRAME_DELAY);
          }
          break;
        case SCREEN3:    // Kết nối wifi thất bại
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Mat ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.drawLine(31, 0 , 0, 31 , 1);
            oled.drawLine(32, 0 , 0, 32 , 1);
            oled.display();
            delay(2000);
            screenOLED = SCREEN7;
          break;
        case SCREEN4:   // Đã kết nối Wifi, đang kết nối Blynk
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.setTextSize(1);
            oled.setCursor(40, 34);
            oled.print("BLYNK");
            oled.setTextSize(1.5);
            oled.setCursor(40, 51);
            oled.print("Dang ket noi..");                    
            for(int i = 0; i < FRAME_COUNT_blynkOLED; i++) {
              clearRectangle(0, 32, 32, 64);
              oled.drawBitmap(0, 32, blynkOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
          break;
        case SCREEN5:   // Đã kết nối Wifi, Đã kết nối Blynk
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.setTextSize(1);
            oled.setCursor(40, 34);
            oled.print("BLYNK");
            oled.setTextSize(1.5);
            oled.setCursor(40, 51);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 32, blynkOLED[FRAME_COUNT_wifiOLED/2], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.display();
            delay(2000);
            screenOLED = SCREEN1;
            enableShow = ENABLE;
          break;
        case SCREEN6:   // Đã kết nối Wifi, Mat kết nối Blynk
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.setTextSize(1);
            oled.setCursor(40, 34);
            oled.print("BLYNK");
            oled.setTextSize(1.5);
            oled.setCursor(40, 51);
            oled.print("Mat ket noi.");
            oled.drawBitmap(0, 32, blynkOLED[FRAME_COUNT_wifiOLED/2], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.drawLine(31, 32 , 0, 63 , 1);
            oled.drawLine(32, 32 , 0, 64 , 1);
            oled.display();
            delay(2000);
            screenOLED = SCREEN7;
          break;
        case SCREEN7:   // AP mode: van hien thi du lieu local
            screenOLED = SCREEN1;
            enableShow = ENABLE;
            break;
          case SCREEN8:    // auto : on
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(20, 5);
            oled.print("Tu Dong Canh Bao");
            oled.setCursor(20, 15);
            oled.print("va Dieu Khien:");
            oled.setTextSize(2);
            oled.setCursor(36, 36);
            oled.print("DISABLE");
            for(int i = 0; i < FRAME_COUNT_autoOnOLED; i++) {
              clearRectangle(0, 25, 32, 48);
              oled.drawBitmap(0, 25, autoOnOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
            clearRectangle(36, 36, 128, 64);
            oled.setCursor(36, 36);
            oled.print("ENABLE");
            oled.display();
            delay(2000);
            screenOLED = SCREEN1;
            enableShow = ENABLE;
            break;
          case SCREEN9:     // auto : off
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(20, 5);  
            oled.print("Tu Dong Canh Bao");
            oled.setCursor(20, 15);
            oled.print("va Dieu Khien:");
            oled.setTextSize(2);
            oled.setCursor(36, 36);  
            oled.print("ENABLE");
            for(int i = 0; i < FRAME_COUNT_autoOffOLED; i++) {
              clearRectangle(0, 25, 32, 48);  
              oled.drawBitmap(0, 25, autoOffOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
            clearRectangle(36, 36, 128, 64);  
            oled.setCursor(36, 36);
            oled.print("DISABLE"); 
            oled.display();    
            delay(2000);
            screenOLED = SCREEN1;  
            enableShow = ENABLE;
            break;
          case SCREEN10:  // gui du lieu len blynk
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 20);
            oled.print("Gui du lieu");
            oled.setCursor(40, 32);
            oled.print("den BLYNK"); 
            for(int i = 0; i < FRAME_COUNT_sendDataOLED; i++) {
                clearRectangle(0, 0, 32, 64);
                oled.drawBitmap(0, 16, sendDataOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
                oled.display();
                delay(FRAME_DELAY);
            } 
            delay(1000);
            screenOLED = SCREEN1; 
            enableShow = ENABLE;
            break;
          case SCREEN11:   // khoi dong lai
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(0, 20);
            oled.print("Khoi dong lai");
            oled.setCursor(0, 32);
            oled.print("Vui long doi ..."); 
            oled.display();
            break;
          default : 
            delay(500);
            break;
      } 
      delay(10);
  }
}
//-----------------Kết nối STA wifi, chuyển sang wifi AP nếu kết nối thất bại ----------------------- 
void connectSTA() {
  if (Essid.length() <= 1 || apModeStarted) return;

  Serial.print("Connecting WiFi: ");
  Serial.println(Essid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(Essid.c_str(), Epass.c_str());

  int countConnect = 0;
  while (WiFi.status() != WL_CONNECTED && countConnect < 20 && !apModeStarted) {
    vTaskDelay(pdMS_TO_TICKS(500));
    countConnect++;
  }

  wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiConnected) {
    Serial.print("WiFi Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connect attempt failed.");
  }
}

// Task mạng độc lập: Wi-Fi/Blynk lỗi không làm dừng hệ thống local
void TaskNetwork(void *pvParameters) {
  uint8_t wifiFailCount = 0;
  bool blynkConfigured = false;
  bool timerConfigured = false;

  while (1) {
    if (restartRequested && (int32_t)(millis() - restartRequestAt) >= 0) {
      ESP.restart();
    }

    if (apModeStarted) {
      wifiConnected = false;
      blynkOnline = false;
      blynkConnect = false;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    // Chưa có Wi-Fi đã lưu -> vào AP cấu hình, local vẫn chạy
    if (Essid.length() <= 1 || Essid == "BLK") {
      connectAPMode();
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    // Mất Wi-Fi -> thử reconnect trong task riêng
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      blynkOnline = false;
      blynkConnect = false;
      connectSTA();

      if (WiFi.status() != WL_CONNECTED) {
        wifiFailCount++;
        if (wifiFailCount >= 3) {
          Serial.println("WiFi failed 3 times -> AP mode");
          connectAPMode();
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
        continue;
      }
      wifiFailCount = 0;
    }

    wifiConnected = true;

    // Wi-Fi có nhưng Blynk lỗi: chỉ retry Blynk, không phá hoạt động local
    if (Etoken.length() > 5) {
      if (!blynkConfigured) {
        memset(BLYNK_AUTH_TOKEN, 0, sizeof(BLYNK_AUTH_TOKEN));
        strncpy(BLYNK_AUTH_TOKEN, Etoken.c_str(), sizeof(BLYNK_AUTH_TOKEN) - 1);
        Blynk.config(BLYNK_AUTH_TOKEN);
        blynkConfigured = true;
      }

      if (!Blynk.connected()) {
        blynkConnect = Blynk.connect(3000);
      } else {
        blynkConnect = true;
      }
      blynkOnline = Blynk.connected();

      if (blynkOnline && TaskBlynk_handle == NULL) {
        xTaskCreatePinnedToCore(TaskBlynk, "TaskBlynk", 1024 * 16, NULL, 9, &TaskBlynk_handle, 0);
      }
      if (blynkOnline && !timerConfigured) {
        timer.setInterval(1000L, myTimer);
        timerConfigured = true;
      }
    } else {
      blynkOnline = false;
      blynkConnect = false;
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

//--------------------------- chuyển đổi chế độ AP --------------------------- 
void connectAPMode() {
  wifiConnected = false;
  blynkOnline = false;
  blynkConnect = false;
  if (Blynk.connected()) Blynk.disconnect();
  // Không cho chế đô AP chạy nhiều lần
  if(apModeStarted) return;
  apModeStarted = true;
  Serial.println("Starting AP Mode...");
  // Tắt STA trước khi bật AP
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  vTaskDelay(pdMS_TO_TICKS(1000));
  // Chuyển sang AP mode
  WiFi.mode(WIFI_AP);
  bool result = WiFi.softAP(ssidAP, passwordAP);
  if(result) {
    Serial.println("AP Started!");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
  }
  else {
    Serial.println("AP Start Failed!");
    return;
  }
  // Trang chủ
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  // Gửi dữ liệu cũ
  server.on("/data_before", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getJsonData();
    request->send(200, "application/json", json);
  });
  // Nhận dữ liệu từ web
  server.on("/post_data", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!configSaveOK) {
      request->send(400, "text/plain", "INVALID DATA");
      return;
    }

    request->send(200, "text/plain", "SUCCESS");
    enableShow = DISABLE;
    screenOLED = SCREEN11;

    // De response gui xong roi moi restart, khong block callback cua web server.
    restartRequested = true;
    restartRequestAt = millis() + 1500;
  }, NULL, getDataFromClient);
  // Start server
  server.begin();
  screenOLED = SCREEN1;
  enableShow = ENABLE;
  Serial.println("AP MODE READY");
}
//------------------- Hàm đọc dữ liệu từ client gửi từ HTTP_POST "/post_data" -------------------
void getDataFromClient(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  static String body;

  if (index == 0) {
    body = "";
    body.reserve(total + 1);
    configSaveOK = false;
  }

  for (size_t i = 0; i < len; i++) {
    body += (char)data[i];
  }

  // AsyncWebServer co the chia body thanh nhieu goi; chi parse khi da nhan du.
  if (index + len < total) return;

  JSONVar myObject = JSON.parse(body);
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Config JSON invalid");
    return;
  }

  if(myObject.hasOwnProperty("ssid"))
    Essid = (const char*) myObject["ssid"];
  if(myObject.hasOwnProperty("pass"))
    Epass = (const char*) myObject["pass"];
  if(myObject.hasOwnProperty("token"))
    Etoken = (const char*) myObject["token"];
  if(myObject.hasOwnProperty("tempThreshold1"))
    EtempThreshold1 = (int) myObject["tempThreshold1"];
  if(myObject.hasOwnProperty("tempThreshold2"))
    EtempThreshold2 = (int) myObject["tempThreshold2"];
  if(myObject.hasOwnProperty("humiThreshold1"))
    EhumiThreshold1 = (int) myObject["humiThreshold1"];
  if(myObject.hasOwnProperty("humiThreshold2"))
    EhumiThreshold2 = (int) myObject["humiThreshold2"];
  if(myObject.hasOwnProperty("dustThreshold1"))
    EdustThreshold1 = (int) myObject["dustThreshold1"];
  if(myObject.hasOwnProperty("dustThreshold2"))
    EdustThreshold2 = (int) myObject["dustThreshold2"];

  // Kiem tra nguong truoc khi ghi EEPROM.
  if (EtempThreshold1 < 0 || EtempThreshold2 > 60 || EtempThreshold1 >= EtempThreshold2 ||
      EhumiThreshold1 < 0 || EhumiThreshold2 > 100 || EhumiThreshold1 >= EhumiThreshold2 ||
      EdustThreshold1 < 0 || EdustThreshold2 > 500 || EdustThreshold1 >= EdustThreshold2 ||
      Essid.length() == 0) {
    Serial.println("Config values invalid");
    return;
  }

  writeEEPROM();
  configSaveOK = true;
}
// ------------ Hàm in các giá trị cài đặt ------------
void printValueSetup() {
    Serial.print("ssid = ");
    Serial.println(Essid);
    Serial.print("pass length = ");
    Serial.println(Epass.length());
    Serial.print("token length = ");
    Serial.println(Etoken.length());
    Serial.print("tempThreshold1 = ");
    Serial.println(EtempThreshold1);
    Serial.print("tempThreshold2 = ");
    Serial.println(EtempThreshold2);
    Serial.print("humiThreshold1 = ");
    Serial.println(EhumiThreshold1);
    Serial.print("humiThreshold2 = ");
    Serial.println(EhumiThreshold2);
    Serial.print("dustThreshold1 = ");
    Serial.println(EdustThreshold1);
    Serial.print("dustThreshold2 = ");
    Serial.println(EdustThreshold2);
    Serial.print("autoWarning = ");
    Serial.println(autoWarning);
}
//-------- Hàm tạo biến JSON để gửi đi khi có lời yêu cầu HTTP_GET "/" --------
String getJsonData() {
  JSONVar myObject;
  myObject["ssid"]  = Essid;
  myObject["pass"]  = Epass;
  myObject["token"] = Etoken;
  myObject["tempThreshold1"] = EtempThreshold1;
  myObject["tempThreshold2"] = EtempThreshold2;
  myObject["humiThreshold1"] = EhumiThreshold1;
  myObject["humiThreshold2"] = EhumiThreshold2;
  myObject["dustThreshold1"] = EdustThreshold1;
  myObject["dustThreshold2"] = EdustThreshold2;
  String jsonData = JSON.stringify(myObject);
  return jsonData;
}
//--------------------------------Task Blynk-------------------------------------
//----------------------------- HÀM TỰ ĐỘNG CẢNH BÁO--------------------------------
void TaskAutoWarning(void *pvParameters)  {
    vTaskDelay(pdMS_TO_TICKS(20000));
    while(1) {
      // Chi danh gia tu dong khi AUTO bat. Ham gui event co co che chong lap.
      if(autoWarning == 1 && blynkOnline) {
          check_air_quality_and_send_to_blynk(ENABLE, tempValue, humiValue, dustValue);
      }
      vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
//----------------------- Gửi giá trị dữ liệu đến Blynk sau mỗi 2 giây--------
void myTimer() {
    Blynk.virtualWrite(V0, tempValue);  
    Blynk.virtualWrite(V1, humiValue);
    Blynk.virtualWrite(V2, dustValue);
    Blynk.virtualWrite(V4, autoWarning); 
    // Đồng bộ trạng thái relay lên Blynk
    Blynk.virtualWrite(V5, relay1State);
    Blynk.virtualWrite(V6, relay2State);
    Blynk.virtualWrite(V7, relay3State);
    Blynk.virtualWrite(V8, relay4State); 
    Blynk.virtualWrite(V9, relay5State);
}
//--------------Nút đọc từ BLYNK và gửi thông báo trở lại Blynk-----------------------
int checkAirQuality = 0;
BLYNK_WRITE(V3) {
    enableShow = DISABLE;
    checkAirQuality = param.asInt();
    if(checkAirQuality == 1) {
      check_air_quality_and_send_to_blynk(DISABLE, tempValue, humiValue, dustValue);
      screenOLED = SCREEN10;
    } 
}
//------------------------- kiểm tra autoWarning từ BLYNK  -----------------------
BLYNK_WRITE(V4) {
    enableShow = DISABLE;
    autoWarning = param.asInt();
    if(autoWarning == 0) {
      screenOLED = SCREEN9;
      // Khi tắt Auto Warning, tắt tất cả relay tự động
      for (int i = 0; i < 5; i++) {
        if (!buttons[i].manualControl) {
          relayControlFuncs[i](false);
          buttons[i].relayState = false;
        }
      }
    } 
    else {
      screenOLED = SCREEN8;
      // Khi bật Auto Warning, reset manualControl để hệ thống tự động hoàn toàn
      for (int i = 0; i < 5; i++) {
        buttons[i].manualControl = false;
      }
    }
}
//---------------------------Task chuyển đổi AP sang STA---------------------------
void TaskBlynk(void *pvParameters) {
  while(1) {
    if (Blynk.connected()) {
      blynkOnline = true;
      blynkConnect = true;
      Blynk.run();
      timer.run();

      if (blynkEventQueue != NULL) {
        BlynkEventMessage item;
        while (xQueueReceive(blynkEventQueue, &item, 0) == pdTRUE) {
          Blynk.logEvent(item.eventCode, item.message);
        }
      }

      vTaskDelay(pdMS_TO_TICKS(10));
    } else {
      blynkOnline = false;
      blynkConnect = false;
      vTaskDelay(pdMS_TO_TICKS(250));
    }
  }
}

// * Các hàm liên quan đến lưu dữ liệu cài đặt vào EEPROM
//--------------------------- Read Eeprom  --------------------------------
const int EEPROM_MAGIC_ADDR = 190;
const uint8_t EEPROM_MAGIC_V = 'V';
const uint8_t EEPROM_MAGIC_2 = '2';

String readEEPROMString(int startAddr, int maxLen) {
  String value = "";
  value.reserve(maxLen);

  for (int i = 0; i < maxLen; i++) {
    uint8_t c = EEPROM.read(startAddr + i);
    if (c == 0 || c == 0xFF) break;
    value += (char)c;
  }
  return value;
}

void writeEEPROMString(int startAddr, int maxLen, const String &value) {
  for (int i = 0; i < maxLen; i++) {
    EEPROM.write(startAddr + i, 0);
  }

  int count = min((int)value.length(), maxLen - 1);
  for (int i = 0; i < count; i++) {
    EEPROM.write(startAddr + i, value[i]);
  }
}

void validateStoredThresholds() {
  if (EtempThreshold1 < 0 || EtempThreshold2 > 60 || EtempThreshold1 >= EtempThreshold2) {
    EtempThreshold1 = 20;
    EtempThreshold2 = 32;
  }

  if (EhumiThreshold1 < 0 || EhumiThreshold2 > 100 || EhumiThreshold1 >= EhumiThreshold2) {
    EhumiThreshold1 = 40;
    EhumiThreshold2 = 75;
  }

  if (EdustThreshold1 < 0 || EdustThreshold2 > 500 || EdustThreshold1 >= EdustThreshold2) {
    EdustThreshold1 = 40;
    EdustThreshold2 = 150;
  }
}

// --------------------------- Read EEPROM ---------------------------
void readEEPROM() {
  bool newLayout = (EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC_V &&
                    EEPROM.read(EEPROM_MAGIC_ADDR + 1) == EEPROM_MAGIC_2);

  Essid = readEEPROMString(0, 32);

  if (newLayout) {
    // V2 layout: SSID 32B, password 64B, token 64B.
    Epass  = readEEPROMString(32, 64);
    Etoken = readEEPROMString(96, 64);
  } else {
    // Tuong thich du lieu cua firmware cu.
    Epass  = readEEPROMString(32, 32);
    Etoken = readEEPROMString(64, 32);
  }

  if(Essid.length() == 0) Essid = "BLK";

  EtempThreshold1 = EEPROM.read(200);
  EtempThreshold2 = EEPROM.read(201);
  EhumiThreshold1 = EEPROM.read(202);
  EhumiThreshold2 = EEPROM.read(203);
  EdustThreshold1 = EEPROM.read(204) * 100 + EEPROM.read(205);
  EdustThreshold2 = EEPROM.read(206) * 100 + EEPROM.read(207);
  // An toan khi khoi dong: luon bat dau o MANUAL.
  // Mat Wi-Fi khi dang chay KHONG thay doi autoWarning.
  autoWarning = 0;

  validateStoredThresholds();
  printValueSetup();
}

// ------------------------ Clear EEPROM ------------------------
void clearEeprom() {
  for (int i = 0; i < 250; ++i) {
    EEPROM.write(i, 0);
  }
}

// -------------------- Ghi data vao EEPROM ------------------
void writeEEPROM() {
  clearEeprom();

  // V2 layout. Gioi han de khong ghi tran sang vung ke tiep.
  writeEEPROMString(0, 32, Essid);     // SSID toi da 31 ky tu
  writeEEPROMString(32, 64, Epass);    // Password toi da 63 ky tu
  writeEEPROMString(96, 64, Etoken);   // Blynk token toi da 63 ky tu

  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_V);
  EEPROM.write(EEPROM_MAGIC_ADDR + 1, EEPROM_MAGIC_2);

  EEPROM.write(200, EtempThreshold1);
  EEPROM.write(201, EtempThreshold2);
  EEPROM.write(202, EhumiThreshold1);
  EEPROM.write(203, EhumiThreshold2);
  EEPROM.write(204, EdustThreshold1 / 100);
  EEPROM.write(205, EdustThreshold1 % 100);
  EEPROM.write(206, EdustThreshold2 / 100);
  EEPROM.write(207, EdustThreshold2 % 100);
  EEPROM.write(210, 0);                // Khoi dong lai luon MANUAL

  EEPROM.commit();
  Serial.println("write eeprom");
}

/**
 * @brief Kiểm tra chất lượng không khí và gửi lên BLYNK
 *
 * @param autoWarning auto Warning
 * @param temp Nhiệt độ hiện tại    *C
 * @param humi Độ ẩm hiện tại        %
 * @param dust bụi PM2.5 hiện tại    ug/m3
 */
void check_air_quality_and_send_to_blynk(bool autoWarning, int temp, int humi, int dust) {
  String notifications = "";
  int tempIndex = 0;
  int dustIndex = 0;
  int humiIndex = 0;
  if(dht11ReadOK ==  true) {
  if(autoWarning == 0) {
    if(temp < EtempThreshold1 )tempIndex = 1;
    else if(temp >= EtempThreshold1 && temp <=  EtempThreshold2)  tempIndex = 2;
    else tempIndex = 3;
    if(humi < EhumiThreshold1 ) humiIndex = 1;
    else if(humi >= EhumiThreshold1 && humi <= EhumiThreshold2)   humiIndex = 2;
    else humiIndex = 3;

    if(dust < EdustThreshold1 ) dustIndex = 1;
    else if(dust >= EdustThreshold1 && dust <= EdustThreshold2)   dustIndex = 2;
    else dustIndex = 3;
    
    notifications = snTemp[tempIndex] + String(temp) + "*C . " + snHumi[humiIndex] + String(humi) + "% . " + snDust[dustIndex] + String(dust) + "ug/m3 . " ;  
    
    queueBlynkEvent("check_data", notifications);
  } else {
    if(temp < EtempThreshold1 )tempIndex = 1;
    else if(temp >= EtempThreshold1 && temp <=  EtempThreshold2)  tempIndex = 0;
    else tempIndex = 3;
    if(humi < EhumiThreshold1 ) humiIndex = 1;
    else if(humi >= EhumiThreshold1 && humi <= EhumiThreshold2)   humiIndex = 0;
    else humiIndex = 3;
    if(dust < EdustThreshold1 ) dustIndex = 0;
    else if(dust >= EdustThreshold1 && dust <= EdustThreshold2)   dustIndex = 2;
    else dustIndex = 3;
    if(tempIndex == 0 && humiIndex == 0 && dustIndex == 0)
      notifications = "";
    else {
      if(tempIndex != 0) notifications = notifications + snTemp[tempIndex] + String(temp) + "*C . ";
      if(humiIndex != 0) notifications = notifications + snHumi[humiIndex] + String(humi) + "% . " ;
      if(dustIndex != 0) notifications = notifications + snDust[dustIndex] + String(dust) + "ug/m3 . " ;
      static String lastAutoNotification = "";
      static unsigned long lastAutoNotificationAt = 0;
      unsigned long now = millis();
      if (notifications != lastAutoNotification ||
          (now - lastAutoNotificationAt) >= 300000UL) {
        queueBlynkEvent("auto_warning", notifications);
        lastAutoNotification = notifications;
        lastAutoNotificationAt = now;
      }
    }
  }
  Serial.println(notifications);
  }
}


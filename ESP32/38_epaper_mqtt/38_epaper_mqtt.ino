#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>
#include "epd2in9b_V4.h"

// ==========================================
// 1. 硬體腳位與基本設定
// ==========================================
const int DHT_PIN   = 14;
const int LIGHT_PIN = 33;

// MQTT 控制的三顆 LED 腳位
const int LED_GREEN  = 15;
const int LED_YELLOW = 2;
const int LED_RED    = 4;

// MQTT Topic 設定
const char* MQTT_GREEN_TOPIC  = "Hsiang/class305/led/green";
const char* MQTT_YELLOW_TOPIC = "Hsiang/class305/led/yellow";
const char* MQTT_RED_TOPIC    = "Hsiang/class305/led/red";
const char* MQTT_DATA_TOPIC   = "Hsiang/class305/data";

// ==========================================
// 2. WiFi / MQTT 設定
// ==========================================
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_SERVER   = "mqttgo.io";
const uint16_t MQTT_PORT  = 1883;

// 時間間隔設定
const unsigned long UPDATE_INTERVAL = 60000UL; // 電子紙 60 秒更新一次
const unsigned long MQTT_INTERVAL   = 10000UL; // MQTT 資料 10 秒發送一次

unsigned long lastEPDUpdate   = 0;
unsigned long lastMQTTPublish  = 0;
unsigned long lastMQTTAttempt  = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
SimpleDHT11 dht11(DHT_PIN);

// 感測器數據
int tempC = 0;
int hum   = 0;
int light = 0;
bool sensorOK = false;

// ==========================================
// 3. E-Paper 畫面繪製相關
// ==========================================
constexpr int BYTES_PER_ROW = EPD_WIDTH / 8;
constexpr int IMAGE_BYTES   = EPD_WIDTH * EPD_HEIGHT / 8;
uint8_t blackImage[IMAGE_BYTES];
uint8_t redImage[IMAGE_BYTES];

Epd epd;

enum PixelColor { WHITE_PIXEL, BLACK_PIXEL, RED_PIXEL };

// --- 點陣圖資 (Bitmap Data) ---
const uint8_t PROGMEM icon_temp_black[] = {
  0x03, 0xC0, 0x04, 0x20, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10,
  0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10,
  0x08, 0x10, 0x08, 0x10, 0x10, 0x08, 0x20, 0x04, 0x20, 0x04, 0x40, 0x02,
  0x40, 0x02, 0x20, 0x04, 0x10, 0x08, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00
};

const uint8_t PROGMEM icon_temp_red[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80,
  0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
  0x01, 0x80, 0x01, 0x80, 0x03, 0xC0, 0x07, 0xE0, 0x0F, 0xF0, 0x0F, 0xF0,
  0x0F, 0xF0, 0x07, 0xE0, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t PROGMEM icon_humi_black[] = {
  0x00, 0x10, 0x00, 0x00, 0x38, 0x00, 0x00, 0x38, 0x00, 0x00, 0x7C, 0x00,
  0x00, 0xEE, 0x00, 0x01, 0xC7, 0x00, 0x01, 0x83, 0x80, 0x03, 0x81, 0xC0,
  0x07, 0x00, 0xE0, 0x0E, 0x00, 0x70, 0x1C, 0x00, 0x38, 0x1C, 0x00, 0x38,
  0x38, 0x00, 0x1C, 0x38, 0x00, 0x1C, 0x38, 0x00, 0x1C, 0x38, 0x00, 0x1C,
  0x1C, 0x00, 0x38, 0x1C, 0x00, 0x38, 0x0E, 0x00, 0x70, 0x07, 0x00, 0xE0,
  0x03, 0x81, 0xC0, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t PROGMEM icon_humi_red[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x03, 0xFF, 0xC0,
  0x03, 0xFF, 0xC0, 0x03, 0xFF, 0xC0, 0x01, 0xFF, 0x80, 0x00, 0xFF, 0x00,
  0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t PROGMEM icon_light_black[] = {
  0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x04, 0x10, 0x20, 0x02, 0x00, 0x40,
  0x01, 0x38, 0x80, 0x00, 0x44, 0x00, 0x00, 0x82, 0x00, 0x01, 0x01, 0x00,
  0x39, 0x00, 0x9C, 0x01, 0x00, 0x80, 0x01, 0x00, 0x80, 0x01, 0x00, 0x80,
  0x39, 0x00, 0x9C, 0x01, 0x00, 0x80, 0x00, 0x82, 0x00, 0x00, 0x44, 0x00,
  0x01, 0x38, 0x80, 0x02, 0x00, 0x40, 0x04, 0x10, 0x20, 0x00, 0x10, 0x00,
  0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t PROGMEM icon_light_red[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x38, 0x00, 0x00, 0x38, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t FONT_5X7[][5] PROGMEM = {
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, {0x42,0x61,0x51,0x49,0x46},
  {0x21,0x41,0x45,0x4B,0x31}, {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03}, {0x36,0x49,0x49,0x49,0x36},
  {0x06,0x49,0x49,0x29,0x1E}, {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
  {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41},
  {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
  {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},
  {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
  {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E},
  {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
  {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F},
  {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
};

// --- E-Paper 繪圖輔助函式 ---
void setPixel(int x, int y, PixelColor color) {
  if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) return;
  int index = y * BYTES_PER_ROW + (x / 8);
  uint8_t mask = 0x80 >> (x % 8);
  if (color == BLACK_PIXEL) {
    blackImage[index] &= (uint8_t)~mask;
    redImage[index]   |= mask;
  } else if (color == RED_PIXEL) {
    blackImage[index] |= mask;
    redImage[index]   &= (uint8_t)~mask;
  } else {
    blackImage[index] |= mask;
    redImage[index]   |= mask;
  }
}

void fillRect(int x, int y, int w, int h, PixelColor color) {
  for (int yy = y; yy < y + h; ++yy)
    for (int xx = x; xx < x + w; ++xx) setPixel(xx, yy, color);
}

void drawRect(int x, int y, int w, int h, PixelColor color) {
  for (int xx = x; xx < x + w; ++xx) { setPixel(xx, y, color); setPixel(xx, y + h - 1, color); }
  for (int yy = y; yy < y + h; ++yy) { setPixel(x, yy, color); setPixel(x + w - 1, yy, color); }
}

void drawBitmap(int x, int y, const uint8_t *bitmap, int w, int h, PixelColor color) {
  int byteWidth = (w + 7) / 8;
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        setPixel(x + i, y + j, color);
      }
    }
  }
}

int glyphIndex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char* p = strchr(letters, c);
  return p ? 10 + (int)(p - letters) : -1;
}

void drawChar(int x, int y, char c, int scale, PixelColor color) {
  if (c == '%') {
    fillRect(x, y, scale, scale, color); 
    fillRect(x + 4 * scale, y + 6 * scale, scale, scale, color);
    for (int i = 0; i < 7; ++i) fillRect(x + (i % 2 ? 3 : 2) * scale, y + (i + 1) * scale, scale, scale, color);
    return;
  }
  if (c == '_') { fillRect(x, y + 6 * scale, 5 * scale, scale, color); return; }
  int gi = glyphIndex(c); if (gi < 0) return;
  for (int col = 0; col < 5; ++col) {
    uint8_t bits = pgm_read_byte(&FONT_5X7[gi][col]);
    for (int row = 0; row < 7; ++row) if (bits & (1 << row)) fillRect(x + col * scale, y + row * scale, scale, scale, color);
  }
}

void drawText(int x, int y, const char* text, int scale, PixelColor color) {
  while (*text) { drawChar(x, y, *text++, scale, color); x += 6 * scale; }
}

void drawCard(int y, const char* label, const char* value, const char* unit, int iconType) {
  drawRect(6, y, 116, 68, BLACK_PIXEL);
  fillRect(6, y, 7, 68, RED_PIXEL);
  
  if (iconType == 1) { // 溫度
    drawBitmap(18, y + 22, icon_temp_black, 16, 24, BLACK_PIXEL);
    drawBitmap(18, y + 22, icon_temp_red, 16, 24, RED_PIXEL);
  } else if (iconType == 2) { // 濕度
    drawBitmap(16, y + 22, icon_humi_black, 24, 24, BLACK_PIXEL);
    drawBitmap(16, y + 22, icon_humi_red, 24, 24, RED_PIXEL);
  } else if (iconType == 3) { // 光線
    drawBitmap(16, y + 22, icon_light_black, 24, 24, BLACK_PIXEL);
    drawBitmap(16, y + 22, icon_light_red, 24, 24, RED_PIXEL);
  }

  drawText(46, y + 10, label, 1, BLACK_PIXEL);
  drawText(46, y + 26, value, 3, BLACK_PIXEL);
  drawText(100, y + 42, unit, 1, RED_PIXEL);
}

void updateEPDDisplay() {
  memset(blackImage, 0xFF, sizeof(blackImage));
  memset(redImage, 0xFF, sizeof(redImage));

  // 標題列
  fillRect(0, 0, 128, 28, RED_PIXEL);
  drawText(12, 8, "E-PAPER DATA", 1, WHITE_PIXEL);

  // 感測器文字緩衝區
  char bufT[10], bufH[10], bufL[10];
  if (sensorOK) {
    snprintf(bufT, sizeof(bufT), "%d", tempC);
    snprintf(bufH, sizeof(bufH), "%d", hum);
  } else {
    snprintf(bufT, sizeof(bufT), "--");
    snprintf(bufH, sizeof(bufH), "--");
  }
  snprintf(bufL, sizeof(bufL), "%d", light);

  // 繪製三張數值卡片
  drawCard(34,  "TEMP",  bufT, "C", 1);
  drawCard(108, "HUMI",  bufH, "%", 2);
  drawCard(182, "LIGHT", bufL, "%", 3);

  // 傳送點陣資訊給 E-Paper 刷新
  epd.Init();
  epd.Display(blackImage, redImage);
  epd.Sleep();
}

// ==========================================
// 4. 感測器讀取與 MQTT 功能
// ==========================================
void readSensors() {
  byte t = 0, h = 0;
  sensorOK = (dht11.read(&t, &h, NULL) == SimpleDHTErrSuccess);
  if (sensorOK) {
    tempC = t;
    hum = h;
  }
  int rawLight = analogRead(LIGHT_PIN);
  light = constrain(map(rawLight, 0, 4095, 0, 100), 0, 100);

  Serial.printf("TEMP=%d C, HUMI=%d %%, LIGHT=%d %%\n", tempC, hum, light);
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting WiFi...");
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
    return true;
  }
  Serial.println("\nWiFi Connection Failed.");
  return false;
}

int ledPinFromTopic(const char* topic) {
  if (strcmp(topic, MQTT_GREEN_TOPIC) == 0) return LED_GREEN;
  if (strcmp(topic, MQTT_YELLOW_TOPIC) == 0) return LED_YELLOW;
  if (strcmp(topic, MQTT_RED_TOPIC) == 0) return LED_RED;
  return -1;
}

// 精準解析 JSON 與單純文字指令
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[64];
  unsigned int n = min(length, (unsigned int)(sizeof(msg) - 1));
  memcpy(msg, payload, n);
  msg[n] = '\0';

  Serial.printf("MQTT Received [%s]: %s\n", topic, msg);

  int pin = ledPinFromTopic(topic);
  if (pin < 0) return;

  bool on = false;
  bool valid = false;

  // 轉換成小寫以方便模糊與精準比對
  String strMsg = String(msg);
  strMsg.toLowerCase();

  if (strMsg.indexOf("on") >= 0 || strMsg.indexOf("1") >= 0) {
    on = true;
    valid = true;
  } else if (strMsg.indexOf("off") >= 0 || strMsg.indexOf("0") >= 0) {
    on = false;
    valid = true;
  }

  if (valid) {
    digitalWrite(pin, on ? HIGH : LOW);
    Serial.printf("--> LED GPIO %d Set to %s\n", pin, on ? "HIGH (ON)" : "LOW (OFF)");
  }
}

bool connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (mqttClient.connected()) return true;

  Serial.print("Connecting MQTT...");
  uint64_t chipid = ESP.getEfuseMac();
  char clientId[40];
  snprintf(clientId, sizeof(clientId), "ESP32_EPD_%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  if (mqttClient.connect(clientId)) {
    Serial.println(" Connected!");
    mqttClient.subscribe(MQTT_GREEN_TOPIC);
    mqttClient.subscribe(MQTT_YELLOW_TOPIC);
    mqttClient.subscribe(MQTT_RED_TOPIC);
    return true;
  }
  Serial.printf(" Failed, rc=%d\n", mqttClient.state());
  return false;
}

bool publishData() {
  if (!mqttClient.connected() || !sensorOK) return false;

  char payload[80];
  snprintf(payload, sizeof(payload), "{\"temp\":%d,\"humi\":%d,\"light\":%d}", tempC, hum, light);

  bool res = mqttClient.publish(MQTT_DATA_TOPIC, payload);
  if (res) {
    Serial.printf("MQTT Published: %s\n", payload);
  }
  return res;
}

// ==========================================
// 5. Setup & Loop
// ==========================================
void setup() {
  Serial.begin(115200);

  // 初始化 LED 腳位
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  pinMode(LIGHT_PIN, INPUT);

  // 讀取首次數據並更新電子紙畫面
  readSensors();
  updateEPDDisplay();

  // 初始化 WiFi & MQTT
  if (connectWiFi()) {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    connectMQTT();
    publishData();
  }

  lastEPDUpdate   = millis();
  lastMQTTPublish = millis();
}

void loop() {
  unsigned long now = millis();

  // 1. WiFi 及 MQTT 連線維護與即時接收指令
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      if (now - lastMQTTAttempt >= 5000UL) {
        lastMQTTAttempt = now;
        connectMQTT();
      }
    } else {
      mqttClient.loop(); // 保持高頻率調用，確保即時響應 LED 開關
    }
  } else {
    static unsigned long lastWiFiAttempt = 0;
    if (now - lastWiFiAttempt >= 10000UL) {
      lastWiFiAttempt = now;
      connectWiFi();
    }
  }

  // 2. MQTT 感測資料每 10 秒發送一次
  if (now - lastMQTTPublish >= MQTT_INTERVAL) {
    lastMQTTPublish = now;
    readSensors();
    publishData();
  }

  // 3. 電子紙 (E-Paper) 每 60 秒刷屏更新一次
  if (now - lastEPDUpdate >= UPDATE_INTERVAL) {
    lastEPDUpdate = now;
    updateEPDDisplay();
  }

  yield();
}
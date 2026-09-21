#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>      // 💡 降級為普通不加密 Client，大幅節省晶片記憶體
#include <HTTPClient.h>
#include <TFT_22_ILI9225.h>
#include <PubSubClient.h>

// === 1. 網路與 MQTTGO 設定 ===
constexpr char WIFI_SSID[] = "H";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";

constexpr char MQTT_SERVER[] = "mqttgo.io";
constexpr uint16_t MQTT_PORT = 1883;

// 🚀 對齊您指定的主題格式 (會自動生成 H/class/溫度、H/class/濕度 等)
#define TOPIC_PREFIX "H/class"

// 🚀 關鍵修正：將網址改為 http 明文傳輸，繞過 SSL 驗證，解決舊核心 NO DATA 的卡死問題
constexpr char WEATHER_URL[] =
  "http://open-meteo.com"
  "&current=temperature_2m,relative_humidity_2m,precipitation,wind_speed_10m,weather_code"
  "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,wind_speed_10m_max"
  "&timezone=Asia%2FTaipei&forecast_days=7";

// === 2. 硬體腳位定義 ===
constexpr int8_t TFT_RST = 26;
constexpr int8_t TFT_RS = 27;
constexpr int8_t TFT_CS = 5;
constexpr int8_t TFT_LED = 25;
constexpr int8_t LIGHT_PIN = 34; 

constexpr uint8_t DAYS = 7;
constexpr uint32_t FETCH_INTERVAL = 60000UL;
constexpr uint32_t PAGE_INTERVAL = 7000UL;

// 色彩主題
constexpr uint16_t BG = COLOR_DARKBLUE;
constexpr uint16_t HEADER = 0x18E3;
constexpr uint16_t PANEL = 0x2124;
constexpr uint16_t LINE = 0x52AA;

// === 3. 物件初始化 ===
SPIClass vspi(VSPI);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_LED);
WiFiClient espClient; 
PubSubClient mqttClient(espClient);

// 數據存儲變數
String dates[DAYS];
int codes[DAYS];
float maxTemps[DAYS];
float minTemps[DAYS];
float rainChances[DAYS];
float maxWinds[DAYS];
float currentTemp = NAN;
float currentHumidity = NAN;
float currentRain = NAN;
float currentWind = NAN;
int currentCode = -1;
int currentLightPercent = 0; 

bool dataValid = false;
String statusText = "STARTING";
uint32_t lastFetch = 0;
uint32_t lastPage = 0;
uint8_t page = 0;
String mqttClientId;

void text(uint16_t x, uint16_t y, const String &value, uint16_t color) {
  tft.drawText(x, y, value, color);
}

// === 4. 圖形自訂美化（TFT_22_ILI9225 語法） ===
void drawSunIcon(uint16_t x, uint16_t y) {
  tft.fillCircle(x + 6, y + 6, 3, COLOR_ORANGE);
  tft.drawLine(x + 6, y, x + 6, y + 2, COLOR_YELLOW);
  tft.drawLine(x + 6, y + 10, x + 6, y + 12, COLOR_YELLOW);
  tft.drawLine(x, y + 6, x + 2, y + 6, COLOR_YELLOW);
  tft.drawLine(x + 10, y + 6, x + 12, y + 6, COLOR_YELLOW);
}

void drawDropIcon(uint16_t x, uint16_t y) {
  tft.fillTriangle(x + 5, y, x + 1, y + 8, x + 9, y + 8, COLOR_CYAN);
  tft.fillCircle(x + 5, y + 8, 4, COLOR_CYAN);
}

void drawWindIcon(uint16_t x, uint16_t y) {
  tft.drawLine(x, y + 3, x + 10, y + 3, COLOR_WHITE);
  tft.drawLine(x + 2, y + 6, x + 12, y + 6, COLOR_WHITE);
  tft.drawLine(x + 1, y + 9, x + 8, y + 9, COLOR_WHITE);
}

void drawLightIcon(uint16_t x, uint16_t y) {
  tft.fillCircle(x + 6, y + 5, 4, COLOR_YELLOW);
  tft.fillRectangle(x + 4, y + 8, x + 8, y + 11, COLOR_GOLD);
  tft.drawLine(x + 5, y + 12, x + 7, y + 12, COLOR_WHITE);
}

// === 5. MQTTGO 連線與數據發布邏輯 ===
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!mqttClient.connected()) {
    Serial.print("Connecting to MQTTGO.io... ");
    if (mqttClient.connect(mqttClientId.c_str())) {
      Serial.println("Connected!");
      statusText = "MQTT ONLINE";
    } else {
      Serial.print("Failed, rc=");
      Serial.println(mqttClient.state());
      statusText = "MQTT RETRY";
    }
  }
}

void publishData() {
  if (!mqttClient.connected() || !dataValid) return;

  char msgBuf[16]; // 💡 正確定義字元陣列長度，確保舊編譯器安全通過

  // 1. 推送溫度至 H/class/溫度
  dtostrf(currentTemp, 4, 1, msgBuf);
  mqttClient.publish(TOPIC_PREFIX "/溫度", msgBuf);

  // 2. 推送濕度至 H/class/濕度
  dtostrf(currentHumidity, 4, 1, msgBuf);
  mqttClient.publish(TOPIC_PREFIX "/濕度", msgBuf);

  // 3. 推送雨量至 H/class/降雨量
  dtostrf(currentRain, 4, 1, msgBuf);
  mqttClient.publish(TOPIC_PREFIX "/降雨量", msgBuf);

  // 4. 推送風速至 H/class/風速
  dtostrf(currentWind, 4, 1, msgBuf);
  mqttClient.publish(TOPIC_PREFIX "/風速", msgBuf);

  // 5. 推送本地光敏亮度至 H/class/亮度
  sprintf(msgBuf, "%d", currentLightPercent);
  mqttClient.publish(TOPIC_PREFIX "/亮度", msgBuf);

  Serial.println("All environment metrics sent to MQTTGO.io!");
}

// === 6. JSON 解析與核心運作 ===
int arrayStartAfter(const String &json, const char *section, const char *key) {
  int sectionPos = json.indexOf(String("\"") + section + "\"");
  if (sectionPos < 0) return -1;
  int keyPos = json.indexOf(String("\"") + key + "\"", sectionPos);
  if (keyPos < 0) return -1;
  int colon = json.indexOf(':', keyPos);
  return json.indexOf('[', colon);
}

bool numberAfter(const String &json, const char *section, const char *key, float &value) {
  int sectionPos = json.indexOf(String("\"") + section + "\"");
  if (sectionPos < 0) return false;
  int keyPos = json.indexOf(String("\"") + key + "\"", sectionPos);
  if (keyPos < 0) return false;
  int colon = json.indexOf(':', keyPos);
  int start = colon + 1;
  while (start < json.length() && (json[start] == ' ' || json[start] == '\n')) ++start;
  int end = start;
  while (end < json.length() && (isDigit(json[end]) || json[end] == '.' || json[end] == '-')) ++end;
  if (end == start) return false;
  value = json.substring(start, end).toFloat();
  return true;
}

bool numberArray(const String &json, const char *section, const char *key, float output[]) {
  int pos = arrayStartAfter(json, section, key);
  if (pos < 0) return false;
  for (uint8_t i = 0; i < DAYS; ++i) {
    while (pos < json.length() && (json[pos] == '[' || json[pos] == ',' || json[pos] == ' ' || json[pos] == '\n')) ++pos;
    int end = pos;
    while (end < json.length() && (isDigit(json[end]) || json[end] == '.' || json[end] == '-')) ++end;
    if (end == pos) return false;
    output[i] = json.substring(pos, end).toFloat();
    pos = end;
  }
  return true;
}

bool stringArray(const String &json, const char *section, const char *key, String output[]) {
  int pos = arrayStartAfter(json, section, key);
  if (pos < 0) return false;
  for (uint8_t i = 0; i < DAYS; ++i) {
    int q1 = json.indexOf('"', pos + 1);
    int q2 = q1 < 0 ? -1 : json.indexOf('"', q1 + 1);
    if (q2 < 0) return false;
    output[i] = json.substring(q1 + 1, q2);
    pos = q2 + 1;
  }
  return true;
}

String dateShort(const String &date) {
  if (date.length() < 10) return "--/--";
  String result = date.substring(5, 10);
  result.replace('-', '/');
  return result;
}

String weatherName(int code) {
  if (code == 0) return "CLEAR";
  if (code <= 3) return "CLOUDY";
  if (code <= 48) return "FOG";
  if (code <= 67) return "RAIN";
  if (code <= 82) return "SHOWER";
  if (code >= 95) return "STORM";
  return "WEATHER";
}

void header(const String &title, const String &status) {
  tft.clear();
  tft.setBackgroundColor(BG);
  tft.setFont(Terminal6x8, MONOSPACE);
  tft.fillRectangle(0, 0, 175, 219, BG);
  tft.fillRectangle(0, 0, 175, 28, HEADER);
  text(24, 7, title, COLOR_WHITE);
  text(10, 205, status, (status == "DATA OK" || status == "MQTT ONLINE") ? COLOR_CYAN : COLOR_ORANGE);
}

void drawCurrentPage() {
  header("KAOHSIUNG  CURRENT", statusText);
  tft.fillRectangle(10, 36, 165, 153, PANEL);
  tft.drawRectangle(10, 36, 165, 153, LINE);
  
  drawSunIcon(18, 46);
  text(36, 49, "TEMPERATURE", COLOR_ORANGE);
  
  drawDropIcon(18, 64);
  text(36, 67, "HUMIDITY", COLOR_CYAN);
  
  drawDropIcon(18, 82);
  text(36, 85, "PRECIPITATION", COLOR_BLUE);
  
  drawWindIcon(18, 100);
  text(36, 103, "WIND SPEED", COLOR_GREENYELLOW);
  
  drawLightIcon(18, 118);
  text(36, 121, "AMBIENT LIGHT", COLOR_YELLOW);

  if (!dataValid) {
    text(112, 85, "NO DATA", COLOR_RED);
    return;
  }
  
  text(125, 49, String(static_cast<int>(round(currentTemp))) + " C", COLOR_WHITE);
  text(125, 67, String(static_cast<int>(round(currentHumidity))) + " %", COLOR_WHITE);
  text(125, 85, String(currentRain, 1) + " mm", COLOR_WHITE);
  text(125, 103, String(static_cast<int>(round(currentWind))) + " km/h", COLOR_WHITE);
  text(125, 121, String(currentLightPercent) + " %", COLOR_WHITE);
  
  text(52, 140, weatherName(currentCode), COLOR_YELLOW);
  tft.drawLine(15, 165, 160, 165, LINE);
  text(34, 178, "7-DAY DETAILS NEXT", COLOR_CYAN);
}

void drawDailyPage() {
  header("7-DAY FORECAST", statusText);
  text(8, 35, "DATE", COLOR_YELLOW);
  text(47, 35, "T/M", COLOR_YELLOW);
  text(86, 35, "RAIN", COLOR_YELLOW);
  text(118, 35, "WIND", COLOR_YELLOW);
  text(150, 35, "SKY", COLOR_YELLOW);
  tft.drawLine(8, 47, 167, 47, LINE);
  if (!dataValid) {
    text(38, 100, "NO WEATHER DATA", COLOR_RED);
    return;
  }
  for (uint8_t i = 0; i < DAYS; ++i) {
    uint16_t y = 54 + i * 21;
    if (i % 2 == 0) tft.fillRectangle(8, y - 2, 167, y + 15, PANEL);
    text(8, y, dateShort(dates[i]), COLOR_WHITE);
    text(47, y, String(static_cast<int>(round(maxTemps[i]))) + "/" + String(static_cast<int>(round(minTemps[i]))), COLOR_ORANGE);
    text(86, y, String(static_cast<int>(round(rainChances[i]))) + "%", rainChances[i] >= 60 ? COLOR_RED : COLOR_CYAN);
    text(118, y, String(static_cast<int>(round(maxWinds[i]))), COLOR_WHITE);
    text(150, y, weatherName(codes[i]).substring(0, 3), codes[i] >= 51 ? COLOR_ORANGE : COLOR_YELLOW);
  }
}

void readLightSensor() {
  int rawLight = analogRead(LIGHT_PIN);
  currentLightPercent = map(rawLight, 0, 4095, 0, 100);
  currentLightPercent = constrain(currentLightPercent, 0, 100);
}

bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    statusText = "WIFI OFFLINE";
    return false;
  }
  WiFiClient client; // 🚀 使用普通不加密的普通連線
  HTTPClient http;
  http.setTimeout(15000);
  http.setReuse(false);
  http.useHTTP10(true);
  if (!http.begin(client, WEATHER_URL)) {
    statusText = "HTTP BEGIN ERR";
    return false;
  }
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    statusText = String("HTTP ") + httpCode;
    http.end();
    return false;
  }
  String json = http.getString();
  http.end();
  float t, h, p, w, c;
  // === 程式碼後半段：排版美化優化版 ===

  float maxT[DAYS], minT[DAYS], rain[DAYS], winds[DAYS], dailyCodes[DAYS];

  // 1. JSON 解析邏輯對齊排版
  bool ok = numberAfter(json, "current", "temperature_2m", t) &&
            numberAfter(json, "current", "relative_humidity_2m", h) &&
            numberAfter(json, "current", "precipitation", p) &&
            numberAfter(json, "current", "wind_speed_10m", w) &&
            numberAfter(json, "current", "weather_code", c) &&
            stringArray(json, "daily", "time", dates) &&
            numberArray(json, "daily", "temperature_2m_max", maxT) &&
            numberArray(json, "daily", "temperature_2m_min", minT) &&
            numberArray(json, "daily", "precipitation_probability_max", rain) &&
            numberArray(json, "daily", "wind_speed_10m_max", winds) &&
            numberArray(json, "daily", "weather_code", dailyCodes);

  if (!ok) {
    statusText = "PARSE ERROR";
    dataValid = false;
    return false;
  }

  // 2. 將數值指派給全域變數
  currentTemp     = t; 
  currentHumidity = h; 
  currentRain     = p; 
  currentWind     = w;
  currentCode     = static_cast<int>(round(c));

  // 3. 跑迴圈將 7 天的預報資料存入陣列
  for (uint8_t i = 0; i < DAYS; ++i) {
    maxTemps[i]    = maxT[i]; 
    minTemps[i]    = minT[i];
    rainChances[i] = rain[i]; 
    maxWinds[i]    = winds[i];
    codes[i]       = static_cast<int>(round(dailyCodes[i]));
  }

  statusText = "DATA OK";
  dataValid = true;
  return true;
}

// === 4. WiFi 連線邏輯排版 ===
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  statusText = "WIFI CONNECTING";
  drawCurrentPage();
  
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts++ < 30) {
    delay(500);
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    statusText = "WIFI FAILED";
  }
}

// === 5. 換頁邏輯排版 ===
void drawPage() {
  readLightSensor();
  if (page == 0) {
    drawCurrentPage(); 
  } else {
    drawDailyPage();
  }
}

// === 6. 初始化 Setup 排版 ===
void setup() {
  Serial.begin(115200);
  pinMode(LIGHT_PIN, INPUT);
  
  // 自動生成唯一的 MQTT 客户端 ID
  mqttClientId = "ESP32_Weather_" + String(random(0, 10000));
  
  // 初始化 ILI9225 螢幕與 SPI 腳位
  vspi.begin(18, -1, 23, TFT_CS);
  tft.begin(vspi);
  
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  tft.setOrientation(0);
  
  // 首次讀取與繪製
  readLightSensor();
  drawCurrentPage();
  
  // 建立網路連線
  connectWiFi();
  
  // 設定 MQTT 連線
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();
  
  // 抓取第一手天氣資料並發布
  dataValid = fetchWeather();
  publishData();
  
  // 刷新最終畫面
  drawPage();
  lastFetch = millis();
  lastPage = millis();
}

// === 7. 主迴圈 Loop 排版 ===
void loop() {
  // 自動檢查 WiFi 並維持 MQTT 連線狀態
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    mqttClient.loop();
  }

  uint32_t now = millis();
  
  // 【定時觸發】每 60 秒更新天氣與 MQTT 發布
  if (now - lastFetch >= FETCH_INTERVAL) {
    lastFetch = now;
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }
    dataValid = fetchWeather();
    readLightSensor();
    publishData();
    drawPage();
  }
  
  // 【定時觸發】每 7 秒切換顯示頁面 (主頁 <-> 7天預報)
  if (now - lastPage >= PAGE_INTERVAL) {
    lastPage = now;
    page = (page == 0) ? 1 : 0;
    drawPage();
  }
  
  // 【即時刷新】每 500 毫秒在背景讀取亮度，並局部重繪畫面數值（避免全螢幕閃爍）
  static uint32_t lastLightUpdate = 0;
  if (now - lastLightUpdate >= 500) {
    lastLightUpdate = now;
    int oldLight = currentLightPercent;
    readLightSensor();
    
    // 如果目前在主頁面、且資料有效、且亮度數值有變動才進行局部重繪
    if (page == 0 && dataValid && oldLight != currentLightPercent) {
      tft.fillRectangle(125, 121, 160, 131, PANEL); // 抹除舊範圍
      text(125, 121, String(currentLightPercent) + " %", COLOR_WHITE);
    }
  }
  
  delay(50);
}

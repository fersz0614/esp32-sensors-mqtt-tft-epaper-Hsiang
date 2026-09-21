#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <SimpleDHT.h>

// =========================
// 1. 硬體腳位
// =========================

const int TFT_SCK   =18; 
const int TFT_MOSI  =23; 
const int TFT_RS    =27; 
const int TFT_RST   =26;
const int TFT_CS    =32; 
// TFT LED 背光：直接接 3.3V，不由 ESP32 GPIO 控制


const int DHT_PIN   = 14;
const int LIGHT_PIN = 33;

// MQTT LED 控制（依 27）
const int LED_GREEN  = 15;
const int LED_YELLOW = 2;
const int LED_RED    = 4;
const int PAGE_BUTTON = 0;   // IO0：按一下切換頁面

const char* MQTT_GREEN_TOPIC  = "Hsiang/class305/led/green";
const char* MQTT_YELLOW_TOPIC = "Hsiang/class305/led/yellow";
const char* MQTT_RED_TOPIC    = "Hsiang/class305/led/red";

#define SPI_FREQUENCY 8000000UL

// =========================
// 2. WiFi / MQTT 設定
// =========================
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_SERVER = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "Hsiang/class305/data";

// MQTT 每 10 秒傳送一次
const unsigned long MQTT_INTERVAL = 10000UL;

// DHT 感測器每 10 秒讀取一次
const unsigned long SENSOR_INTERVAL = 10000UL;

// =========================
// 3. ILI9225 GFX
// =========================
void xfer16(uint16_t v) {
  SPI.transfer(v >> 8);
  SPI.transfer(v);
}

class ILI9225_GFX : public Adafruit_GFX {
public:
  ILI9225_GFX() : Adafruit_GFX(176, 220) {}

  void begin() {
    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_RS, OUTPUT);
    pinMode(TFT_RST, OUTPUT);

    digitalWrite(TFT_CS, HIGH);
    SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
    SPI.setFrequency(SPI_FREQUENCY);

    digitalWrite(TFT_RST, LOW);
    delay(50);
    digitalWrite(TFT_RST, HIGH);
    delay(100);

    cmd(0x01, 0x011C);
    cmd(0x02, 0x0100);
    cmd(0x03, 0x1030);
    cmd(0x08, 0x0808);
    cmd(0x0F, 0x0801);
    cmd(0x20, 0);
    cmd(0x21, 0);
    cmd(0x10, 0);

    cmd(0x11, 0x1B41);
    cmd(0x12, 0x200E);
    cmd(0x13, 0x0D00);
    cmd(0x14, 0x0020);

    delay(50);

    cmd(0x10, 0x0F00);
    delay(50);

    cmd(0x11, 0x1B41);
    cmd(0x12, 0x200E);
    cmd(0x13, 0x0D00);
    cmd(0x14, 0x0020);

    cmd(0x30, 0);
    cmd(0x31, 0x00DB);
    cmd(0x32, 0);
    cmd(0x33, 0);
    cmd(0x34, 0x00DB);
    cmd(0x35, 0);
    cmd(0x36, 0x00AF);
    cmd(0x37, 0);
    cmd(0x38, 0x00DB);
    cmd(0x39, 0);

    cmd(0x07, 0x1017);
    delay(50);
  }

  void fillFast(int x, int y, int w, int h, uint16_t c) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > 176) w = 176 - x;
    if (y + h > 220) h = 220 - y;

    if (w <= 0 || h <= 0) return;

    window(x, y, x + w - 1, y + h - 1);

    for (long i = 0; i < (long)w * h; i++) {
      xfer16(c);
    }

    digitalWrite(TFT_CS, HIGH);
  }

  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if (x < 0 || y < 0 || x >= 176 || y >= 220) return;
    fillFast(x, y, 1, 1, c);
  }


private:
  void cmd(uint8_t c, uint16_t d) {
    digitalWrite(TFT_CS, LOW);
    digitalWrite(TFT_RS, LOW);
    SPI.transfer(c);
    digitalWrite(TFT_RS, HIGH);
    xfer16(d);
    digitalWrite(TFT_CS, HIGH);
  }

  void window(int x0, int y0, int x1, int y1) {
    cmd(0x36, x0);
    cmd(0x37, x1);
    cmd(0x38, y0);
    cmd(0x39, y1);
    cmd(0x20, x0);
    cmd(0x21, y0);

    digitalWrite(TFT_CS, LOW);
    digitalWrite(TFT_RS, LOW);
    SPI.transfer(0x22);
    digitalWrite(TFT_RS, HIGH);
  }
};

ILI9225_GFX tft;
SimpleDHT11 dht11(DHT_PIN);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =========================
// 4. 感測資料
// =========================
int tempC = 0;
int hum = 0;
int light = 0;
bool sensorOK = false;

unsigned long lastRead = 0;
unsigned long lastMQTTPublish = 0;
unsigned long lastMQTTAttempt = 0;

// 狀態文字
String wifiMessage = "CONNECTING";
String mqttMessage = "WAIT";
String screenMessage = "STARTING";
unsigned long topMessageUntil = 0;

// =========================
// 目前時間 / 頁面 / 溫度歷史
// =========================
const long TAIWAN_UTC_OFFSET = 8L * 3600L;
const unsigned long TIME_UPDATE_INTERVAL = 10000UL;
const unsigned long HISTORY_INTERVAL = 10000UL;

uint8_t currentPage = 0;

// IO0 是 NMK99 的 BOOT 按鍵。
// 這一版改用主迴圈輪詢，不使用中斷。
// 原因：GPIO0 同時是 ESP32 BOOT 腳位，輪詢方式較容易控制
// 防彈跳與「按一下只換一次」，也避免 TFT 繪圖期間漏掉中斷事件。
bool pageButtonLastState = HIGH;
unsigned long lastPageSwitch = 0;
const unsigned long PAGE_DEBOUNCE = 120UL;

int16_t tempHistory[61];
int16_t humHistory[61];
int16_t lightHistory[61];
unsigned long lastHistory = 0;
unsigned long lastTimeDraw = 0;

const uint16_t GAUGE_GREEN = 0x05E0;
const uint16_t GAUGE_YELLOW = 0xFFE0;
const uint16_t GAUGE_RED = 0xF800;

// =========================
// 5. 顏色
// =========================
const uint16_t BG    = 0x0000;
const uint16_t PANEL = 0x18E3;
const uint16_t WHITE = 0xFFFF;
const uint16_t RED   = 0xF800;
const uint16_t BLUE  = 0x001F;
const uint16_t CYAN  = 0x07FF;
const uint16_t ORANGE= 0xFD20;
const uint16_t GREEN = 0x07E0;
const uint16_t YELLOW= 0xFFE0;
const uint16_t BLACK = 0x0000;

// =========================
// 6. 圖示
// =========================
void drawThermometer(int x, int y) {
  tft.fillCircle(x, y + 18, 10, RED);
  tft.fillRect(x - 4, y - 15, 8, 34, RED);
  tft.fillRect(x - 2, y - 10, 4, 28, ORANGE);
}

void drawDrop(int x, int y) {
  tft.fillTriangle(x, y - 22, x - 14, y + 7, x + 14, y + 7, BLUE);
  tft.fillCircle(x, y + 4, 14, BLUE);
  tft.fillCircle(x - 4, y - 6, 2, WHITE);
}

void drawSun(int x, int y) {
  tft.fillCircle(x, y, 9, YELLOW);
  tft.drawCircle(x, y, 12, ORANGE);

  for (int i = 0; i < 8; i++) {
    float a = i * 0.785398f;
    tft.drawLine(
      x + (int)(15 * cos(a)),
      y + (int)(15 * sin(a)),
      x + (int)(20 * cos(a)),
      y + (int)(20 * sin(a)),
      ORANGE
    );
  }
}

// =========================
// 7. 狀態列
// =========================
void drawStatusBar() {
  // 最上方 24px：平時顯示 WiFi / MQTT 狀態；
  // 有事件訊息時暫時切換顯示文字，避免佔用最下面的感測區。
  tft.fillRect(0, 0, 175, 24, BLACK);
  tft.setTextSize(1);

  if (millis() < topMessageUntil) {
    tft.setTextColor(ORANGE);
    tft.setCursor(5, 8);
    tft.print(screenMessage);
  } else {
    tft.setTextColor(wifiMessage == "OK" ? GREEN : RED);
    tft.setCursor(5, 8);
    tft.print("WIFI:");
    tft.print(wifiMessage == "OK" ? "O" : "X");

    tft.setTextColor(mqttMessage == "OK" ? GREEN : RED);
    tft.setCursor(65, 8);
    tft.print("MQTT:");
    tft.print(mqttMessage == "OK" ? "O" : "X");

    if (wifiMessage == "OK" && mqttMessage == "OK") {
      tft.setTextColor(GREEN);
      tft.setCursor(113, 8);
      tft.print("OK");
    } else {
      tft.setTextColor(ORANGE);
      tft.setCursor(113, 8);
      tft.print("WAIT");
    }
  }

  tft.drawLine(0, 24, 175, 24, PANEL);
}

// =========================
// 8. 第一頁：環境資訊 + 台灣時間
// =========================
void drawTimeLine() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 20)) {
    tft.setTextColor(ORANGE);
    tft.setTextSize(1);
    tft.setCursor(55, 198);
    tft.print("TIME SYNC...");
    return;
  }

  static const char* days[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };

  char buf[24];
  snprintf(
    buf, sizeof(buf),
    "%02d/%02d %s %02d:%02d",
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    days[timeinfo.tm_wday],
    timeinfo.tm_hour,
    timeinfo.tm_min
  );

  // 時間顯示：09/14 Mon 13:20
  // 使用 1 倍字體，完整放入 176x220 TFT，並置中。
  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.setCursor(43, 198);
  tft.print(buf);
}
void drawScreenPage1() {
  tft.fillFast(0, 0, 176, 220, BG);
  drawStatusBar();

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 29);
  tft.print("ENVIRONMENT");

  tft.setTextSize(1);
  tft.setCursor(148, 34);
  tft.print("1/2");

  // 溫度
  tft.fillRoundRect(7, 48, 162, 36, 5, PANEL);
  drawThermometer(18, 66);
  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.setCursor(34, 60);
  tft.print("TEMPERATURE");
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(104, 54);
  if (sensorOK) tft.print(tempC); else tft.print("--");
  tft.setTextSize(1);
  tft.setCursor(145, 64);
  tft.print("C");

  // 濕度
  tft.fillRoundRect(7, 89, 162, 36, 5, PANEL);
  drawDrop(18, 107);
  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.setCursor(34, 101);
  tft.print("HUMIDITY");
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(104, 95);
  if (sensorOK) tft.print(hum); else tft.print("--");
  tft.setTextSize(1);
  tft.setCursor(145, 105);
  tft.print("%");

  // 光線
  tft.fillRoundRect(7, 130, 162, 36, 5, PANEL);
  drawSun(18, 148);
  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.setCursor(34, 142);
  tft.print("LIGHT");
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(104, 136);
  tft.print(light);
  tft.setTextSize(1);
  tft.setCursor(145, 146);
  tft.print("%");

  // 台灣時間
  tft.drawLine(7, 174, 169, 174, PANEL);
  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.setCursor(59, 181);
  tft.print("TAIWAN TIME");
  tft.fillFast(7, 193, 162, 20, BG);
  drawTimeLine();
}

uint16_t gaugeColorForValue(float v) {
  if (v < 20.0f) return GAUGE_GREEN;
  if (v < 30.0f) return GAUGE_YELLOW;
  return GAUGE_RED;
}

float tempToGaugeAngle(float value) {
  value = constrain(value, 10.0f, 40.0f);
  // 10 = 180 degrees, 40 = 0 degrees
  return 180.0f - ((value - 10.0f) / 30.0f) * 180.0f;
}

void drawGauge() {
  // 上半部 Gauge：10~40°C
  // 10~20 綠、20~30 黃、30~40 紅
  const int cx = 88;
  const int cy = 103;
  const int rOuter = 48;
  const int rInner = 42;

  for (int deg = 180; deg >= 0; --deg) {
    float value = 10.0f +
                  (180.0f - deg) * (30.0f / 180.0f);

    uint16_t c = gaugeColorForValue(value);
    float a = deg * 0.01745329252f;

    int x1 = cx + (int)(cos(a) * rInner);
    int y1 = cy - (int)(sin(a) * rInner);
    int x2 = cx + (int)(cos(a) * rOuter);
    int y2 = cy - (int)(sin(a) * rOuter);

    tft.drawLine(x1, y1, x2, y2, c);

    if ((deg % 2) == 0) {
      tft.drawLine(x1, y1 + 1, x2, y2 + 1, c);
    }
  }

  // 刻度
  for (int v = 10; v <= 40; v += 5) {
    float a = tempToGaugeAngle(v) * 0.01745329252f;
    int x1 = cx + (int)(cos(a) * 39);
    int y1 = cy - (int)(sin(a) * 39);
    int x2 = cx + (int)(cos(a) * 46);
    int y2 = cy - (int)(sin(a) * 46);

    tft.drawLine(x1, y1, x2, y2, WHITE);
  }

  // 10 / 20 / 30 / 40
  tft.setTextColor(WHITE);
  tft.setTextSize(1);

  tft.setCursor(28, 99);
  tft.print("10");

  tft.setCursor(50, 58);
  tft.print("20");

  tft.setCursor(111, 58);
  tft.print("30");

  tft.setCursor(145, 99);
  tft.print("40");

  // 指針
  float value = sensorOK ? tempC : 10;
  float a = tempToGaugeAngle(value) * 0.01745329252f;

  int nx = cx + (int)(cos(a) * 37);
  int ny = cy - (int)(sin(a) * 37);

  tft.drawLine(cx, cy, nx, ny, WHITE);
  tft.drawLine(cx + 1, cy, nx + 1, ny, WHITE);

  tft.fillCircle(cx, cy, 7, WHITE);
  tft.fillCircle(cx, cy, 3, WHITE);

  // 中央數值
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(70, 108);

  if (sensorOK) tft.print(tempC);
  else tft.print("--");

  tft.setTextSize(1);
  tft.setCursor(101, 114);
  tft.print("C");
}

void drawTemperatureChart() {
  // 最近 10 分鐘：溫度、濕度、亮度三條曲線
  // 溫度左軸 10~40°C；濕度/亮度右軸 0~100%。
  const int left = 25;
  const int right = 166;
  const int top = 137;
  const int bottom = 199;

  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.setCursor(25, 126);
  tft.print("LAST 10 MIN");

  tft.setTextColor(CYAN);
  tft.setCursor(90, 126);
  tft.print("T:");
  if (sensorOK) tft.print(tempC); else tft.print("--");
  tft.setTextColor(YELLOW);
  tft.setCursor(119, 126);
  tft.print("H:");
  if (sensorOK) tft.print(hum); else tft.print("--");
  tft.setTextColor(RED);
  tft.setCursor(148, 126);
  tft.print("L:");
  tft.print(light);

  tft.drawRect(left, top, right - left, bottom - top, PANEL);

  // 左軸：溫度 10~40
  for (int v = 10; v <= 40; v += 10) {
    int y = bottom - (v - 10) * (bottom - top) / 30;
    tft.drawLine(left + 1, y, right - 1, y, 0x7BEF);
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.setCursor(3, y - 3);
    tft.print(v);
  }

  // 右軸：濕度/亮度 0~100
  tft.setTextColor(WHITE);
  tft.setCursor(150, bottom - 3);
  tft.print("0");
  tft.setCursor(148, (top + bottom) / 2 - 3);
  tft.print("50");
  tft.setCursor(142, top - 3);
  tft.print("100");

  // X 軸
  for (int i = 0; i <= 5; ++i) {
    int x = left + i * (right - left) / 5;
    tft.drawLine(x, bottom, x, bottom + 2, PANEL);
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    if (i == 5) {
      tft.setCursor(x - 2, 202);
      tft.print("0");
    } else {
      tft.setCursor(x - 7, 202);
      tft.print("-");
      tft.print(10 - i * 2);
    }
  }

  bool prevT = false, prevH = false, prevL = false;
  int ptx = 0, pty = 0, phx = 0, phy = 0, plx = 0, ply = 0;

  for (int i = 0; i < 61; ++i) {
    int x = left + i * (right - left) / 60;

    if (tempHistory[i] >= 10 && tempHistory[i] <= 40) {
      int y = bottom - (tempHistory[i] - 10) * (bottom - top) / 30;
      if (prevT) tft.drawLine(ptx, pty, x, y, CYAN);
      ptx = x; pty = y; prevT = true;
    }

    if (humHistory[i] >= 0 && humHistory[i] <= 100) {
      int y = bottom - humHistory[i] * (bottom - top) / 100;
      if (prevH) tft.drawLine(phx, phy, x, y, YELLOW);
      phx = x; phy = y; prevH = true;
    }

    if (lightHistory[i] >= 0 && lightHistory[i] <= 100) {
      int y = bottom - lightHistory[i] * (bottom - top) / 100;
      if (prevL) tft.drawLine(plx, ply, x, y, RED);
      plx = x; ply = y; prevL = true;
    }
  }
}

void drawScreenPage2() {
  tft.fillFast(0, 0, 176, 220, BG);
  drawStatusBar();

  // 第二頁標題
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 29);
  tft.print("TEMP GAUGE");

  tft.setTextSize(1);
  tft.setCursor(148, 34);
  tft.print("2/2");

  drawGauge();
  drawTemperatureChart();
}

void drawScreen() {
  if (currentPage == 0) {
    drawScreenPage1();
  } else {
    drawScreenPage2();
  }
}

// =========================
// 9. 只更新狀態列
// =========================
void updateStatusBar() {
  drawStatusBar();
}

// =========================
// 10. 顯示 WiFi 連線過程
// =========================
void showConnectionMessage(const String& msg) {
  screenMessage = msg;
  // 狀態訊息暫時佔用最上方狀態列，顯示 2.5 秒後自動回到 WIFI/MQTT 狀態。
  topMessageUntil = millis() + 2500UL;
  drawStatusBar();
}

// =========================
// 11. WiFi 連線
// =========================
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  wifiMessage = "X";
  mqttMessage = "X";
  showConnectionMessage("WIFI CONNECTING...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;

    String msg = "WIFI CONNECTING " + String(attempts);
    showConnectionMessage(msg);

    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiMessage = "OK";
    screenMessage = "WIFI CONNECTED";
    Serial.println("WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // 台灣時區 UTC+8，NTP 自動校時
    configTime(TAIWAN_UTC_OFFSET, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

    updateStatusBar();
    showConnectionMessage("WIFI CONNECTED");
    return true;
  }

  wifiMessage = "X";
  mqttMessage = "X";
  screenMessage = "WIFI FAILED";
  Serial.println("WiFi connection failed.");

  updateStatusBar();
  showConnectionMessage("WIFI FAILED");
  return false;
}

// =========================
// 12. MQTT 連線
// =========================
bool connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    mqttMessage = "X";
    updateStatusBar();
    return false;
  }

  if (mqttClient.connected()) {
    mqttMessage = "OK";
    updateStatusBar();
    return true;
  }

  screenMessage = "MQTT CONNECTING...";
  showConnectionMessage(screenMessage);

  Serial.print("Connecting MQTT... ");

  // 使用 MAC 位址產生唯一 Client ID
  uint64_t chipid = ESP.getEfuseMac();
  char clientId[40];
  snprintf(
    clientId,
    sizeof(clientId),
    "ESP32_%04X%08X",
    (uint16_t)(chipid >> 32),
    (uint32_t)chipid
  );

  if (mqttClient.connect(clientId)) {
    mqttMessage = "OK";
    screenMessage = "MQTT CONNECTED";
    Serial.println("OK");

    // 依 27：三顆 LED 各自獨立 Topic，保留最後狀態。
    mqttClient.subscribe(MQTT_GREEN_TOPIC);
    mqttClient.subscribe(MQTT_YELLOW_TOPIC);
    mqttClient.subscribe(MQTT_RED_TOPIC);

    updateStatusBar();
    showConnectionMessage("MQTT CONNECTED");
    return true;
  }

  mqttMessage = "X";
  screenMessage = "MQTT FAILED";
  Serial.print("FAILED, rc=");
  Serial.println(mqttClient.state());

  updateStatusBar();
  showConnectionMessage("MQTT FAILED");
  return false;
}

// =========================
// MQTT LED 控制（依 27）
// =========================
const char* ledName(const char* topic) {
  if (strcmp(topic, MQTT_GREEN_TOPIC) == 0) return "GLED";
  if (strcmp(topic, MQTT_YELLOW_TOPIC) == 0) return "YLED";
  if (strcmp(topic, MQTT_RED_TOPIC) == 0) return "RLED";
  return nullptr;
}

int ledPinFromTopic(const char* topic) {
  if (strcmp(topic, MQTT_GREEN_TOPIC) == 0) return LED_GREEN;
  if (strcmp(topic, MQTT_YELLOW_TOPIC) == 0) return LED_YELLOW;
  if (strcmp(topic, MQTT_RED_TOPIC) == 0) return LED_RED;
  return -1;
}

bool jsonLedCommand(const char* msg, const char* key, bool &on) {
  const char* p = strstr(msg, key);
  if (!p) return false;
  p = strchr(p, ':');
  if (!p) return false;
  p++;
  while (*p == ' ' || *p == '\t' || *p == '"') p++;
  if (strncasecmp(p, "on", 2) == 0 || strncasecmp(p, "true", 4) == 0 || *p == '1') {
    on = true; return true;
  }
  if (strncasecmp(p, "off", 3) == 0 || strncasecmp(p, "false", 5) == 0 || *p == '0') {
    on = false; return true;
  }
  return false;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[64];
  unsigned int n = min(length, (unsigned int)(sizeof(msg) - 1));
  memcpy(msg, payload, n);
  msg[n] = '\0';

  int pin = ledPinFromTopic(topic);
  const char* name = ledName(topic);
  if (pin < 0 || name == nullptr) return;

  bool on = false;
  bool valid = false;

  if (strchr(msg, '{')) {
    if (pin == LED_GREEN)  valid = jsonLedCommand(msg, "\"gled\"", on);
    if (pin == LED_YELLOW) valid = jsonLedCommand(msg, "\"yled\"", on);
    if (pin == LED_RED)    valid = jsonLedCommand(msg, "\"rled\"", on);
  } else {
    if (strcasecmp(msg, "ON") == 0)  { on = true;  valid = true; }
    if (strcasecmp(msg, "OFF") == 0) { on = false; valid = true; }
  }

  if (!valid) return;

  // 收到封包後第一時間切 GPIO，避免 TFT 繪圖造成 LED 延遲。
  digitalWrite(pin, on ? HIGH : LOW);

  Serial.printf("MQTT LED %s GPIO%d -> %s\n", name, pin, on ? "ON" : "OFF");

  // 保留 27 的 retained 狀態。這裡直接回寫一次。
  if (strchr(msg, '{') && mqttClient.connected()) {
    mqttClient.publish(topic, on ? "ON" : "OFF", true);
  }
}

// =========================
// 13. 讀取 DHT11 + 光線
// =========================
void readSensors() {
  byte t = 0;
  byte h = 0;

  sensorOK = (dht11.read(&t, &h, NULL) == SimpleDHTErrSuccess);

  if (sensorOK) {
    tempC = t;
    hum = h;
  }

  int rawLight = analogRead(LIGHT_PIN);
  light = constrain(map(rawLight, 0, 4095, 0, 100), 0, 100);

  Serial.print("TEMP=");
  Serial.print(tempC);
  Serial.print(" HUMI=");
  Serial.print(hum);
  Serial.print(" LIGHT=");
  Serial.println(light);
}

// =========================
// 14. MQTT JSON 傳送
// JSON:
// {"temp":25,"humi":25,"light":95}
// =========================
bool publishData() {
  if (WiFi.status() != WL_CONNECTED) {
    mqttMessage = "X";
    updateStatusBar();
    return false;
  }

  if (!mqttClient.connected()) {
    mqttMessage = "X";
    updateStatusBar();
    return false;
  }

  if (!sensorOK) {
    Serial.println("DHT sensor invalid, MQTT not published.");
    return false;
  }

  char payload[80];

  snprintf(
    payload,
    sizeof(payload),
    "{\"temp\":%d,\"humi\":%d,\"light\":%d}",
    tempC,
    hum,
    light
  );

  bool result = mqttClient.publish(MQTT_TOPIC, payload);

  if (result) {
    mqttMessage = "OK";
    screenMessage = "MQTT DATA SENT";
    Serial.print("MQTT -> ");
    Serial.println(payload);
  } else {
    mqttMessage = "X";
    screenMessage = "MQTT SEND FAILED";
    Serial.println("MQTT publish failed.");
  }

  updateStatusBar();
  showConnectionMessage(screenMessage);

  return result;
}

// =========================
// 15. Setup
// =========================
void setup() {
  Serial.begin(115200);

  pinMode(LIGHT_PIN, INPUT);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  pinMode(PAGE_BUTTON, INPUT_PULLUP);
  pageButtonLastState = digitalRead(PAGE_BUTTON);

  // 初始化 TFT
  tft.begin();

  // 先顯示啟動畫面
  drawScreen();

  // 先讀一次感測器
  readSensors();
  for (int i = 0; i < 61; ++i) {
    if (sensorOK) {
      tempHistory[i] = constrain(tempC, 10, 40);
      humHistory[i] = constrain(hum, 0, 100);
    } else {
      tempHistory[i] = -127;
      humHistory[i] = -1;
    }
    lightHistory[i] = constrain(light, 0, 100);
  }
  lastHistory = millis();

  // 連線 WiFi
  if (connectWiFi()) {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    connectMQTT();
  }

  // 第一次立即傳送 MQTT
  if (mqttClient.connected()) {
    publishData();
  }

  drawScreen();

  lastRead = millis();
  lastMQTTPublish = millis();
  lastMQTTAttempt = millis();
}

// =========================
// 16. Loop
// =========================
void handlePageButton() {
  // IO0：INPUT_PULLUP，未按 = HIGH，按下 = LOW。
  // 使用「HIGH -> LOW」邊緣判斷，因此長按不會重複換頁。
  bool nowState = digitalRead(PAGE_BUTTON);
  unsigned long now = millis();

  if (pageButtonLastState == HIGH && nowState == LOW) {
    if (now - lastPageSwitch >= PAGE_DEBOUNCE) {
      lastPageSwitch = now;
      currentPage = (currentPage + 1) % 2;

      Serial.printf("IO0 BUTTON PRESSED -> PAGE %d\\n", currentPage + 1);

      // 換頁只在這裡執行一次。
      drawScreen();
    }
  }

  pageButtonLastState = nowState;
}

void updateTemperatureHistory() {
  for (int i = 0; i < 60; ++i) {
    tempHistory[i] = tempHistory[i + 1];
    humHistory[i] = humHistory[i + 1];
    lightHistory[i] = lightHistory[i + 1];
  }

  if (sensorOK) {
    tempHistory[60] = constrain(tempC, 10, 40);
    humHistory[60] = constrain(hum, 0, 100);
  } else {
    tempHistory[60] = -127;
    humHistory[60] = -1;
  }

  lightHistory[60] = constrain(light, 0, 100);
}


// =========================
// 快速更新：不重畫整頁
// =========================
void updatePage1Values() {
  // 只清除三個數值區與時間區，避免整頁重畫。
  tft.fillFast(100, 53, 43, 30, PANEL);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(104, 54);
  if (sensorOK) tft.print(tempC); else tft.print("--");
  tft.setTextSize(1);
  tft.setCursor(145, 64);
  tft.print("C");

  tft.fillFast(100, 94, 43, 30, PANEL);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(104, 95);
  if (sensorOK) tft.print(hum); else tft.print("--");
  tft.setTextSize(1);
  tft.setCursor(145, 105);
  tft.print("%");

  tft.fillFast(100, 135, 43, 30, PANEL);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(104, 136);
  tft.print(light);
  tft.setTextSize(1);
  tft.setCursor(145, 146);
  tft.print("%");

  tft.fillFast(7, 193, 162, 20, BG);
  drawTimeLine();
}

void updatePage2() {
  // 第二頁只更新 Gauge + 折線圖區，不重畫標題與狀態列。
  tft.fillFast(25, 50, 140, 80, BG);
  drawGauge();
  tft.fillFast(0, 122, 176, 98, BG);
  drawTemperatureChart();
}

void loop() {
  unsigned long now = millis();

  // IO0 換頁
  handlePageButton();

  // MQTT 優先處理，LED callback 會立即切換 GPIO
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      mqttMessage = "X";
      if (now - lastMQTTAttempt >= 5000UL) {
        lastMQTTAttempt = now;
        connectMQTT();
      }
    } else {
      mqttMessage = "OK";
      mqttClient.loop();
    }
  }

  // WiFi 狀態
  if (WiFi.status() != WL_CONNECTED) {
    wifiMessage = "X";
    mqttMessage = "X";
    static unsigned long lastWiFiAttempt = 0;
    if (now - lastWiFiAttempt >= 5000UL) {
      lastWiFiAttempt = now;
      if (connectWiFi()) {
        mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
        mqttClient.setCallback(mqttCallback);
        connectMQTT();
      }
    }
  } else {
    wifiMessage = "OK";
  }

  // 每 10 秒更新感測器、時間與折線圖
  if (now - lastRead >= SENSOR_INTERVAL) {
    lastRead = now;

    readSensors();
    updateTemperatureHistory();
    lastHistory = now;
    lastTimeDraw = now;

    if (currentPage == 0) {
      updatePage1Values();
    } else {
      updatePage2();
    }
  }

  // MQTT 感測資料 10 秒一次
  if (now - lastMQTTPublish >= MQTT_INTERVAL) {
    lastMQTTPublish = now;
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqttClient.connected()) connectMQTT();
      if (mqttClient.connected()) publishData();
    }
  }

  // 時間至少每 10 秒更新一次；不在每圈重畫 TFT，避免影響 MQTT。
  yield();
}


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

// =========================
// LED 腳位（依 27_mqtt_retained）
// =========================
const int LED_GREEN  = 15;
const int LED_YELLOW = 2;
const int LED_RED    = 4;

// LED MQTT Topic（依 27_mqtt_retained）
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

// DHT 感測器每 5 秒讀取一次
const unsigned long SENSOR_INTERVAL = 5000UL;

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

    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    digitalWrite(TFT_CS, LOW);
    digitalWrite(TFT_RS, HIGH);

    for (long i = 0; i < (long)w * h; i++) {
      xfer16(c);
    }

    digitalWrite(TFT_CS, HIGH);
    SPI.endTransaction();
  }

  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if (x < 0 || y < 0 || x >= 176 || y >= 220) return;
    fillFast(x, y, 1, 1, c);
  }

private:
  void cmd(uint8_t c, uint16_t d) {
    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    digitalWrite(TFT_CS, LOW);
    digitalWrite(TFT_RS, LOW);
    SPI.transfer(c);
    digitalWrite(TFT_RS, HIGH);
    xfer16(d);
    digitalWrite(TFT_CS, HIGH);
    SPI.endTransaction();
  }

  void window(int x0, int y0, int x1, int y1) {
    cmd(0x36, x0);
    cmd(0x37, x1);
    cmd(0x38, y0);
    cmd(0x39, y1);
    cmd(0x20, x0);
    cmd(0x21, y0);

    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    digitalWrite(TFT_CS, LOW);
    digitalWrite(TFT_RS, LOW);
    SPI.transfer(0x22);
    digitalWrite(TFT_RS, HIGH);
    SPI.endTransaction();
  }
};

ILI9225_GFX tft;
SimpleDHT11 dht11(DHT_PIN);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// LED 保留狀態延後到 loop() 發布，避免在 MQTT callback 內做網路傳送。
// 這樣 GPIO 會在收到封包後立即切換，MQTT callback 不會被阻塞。
volatile bool pendingGreenState = false;
volatile bool pendingYellowState = false;
volatile bool pendingRedState = false;
volatile bool pendingGreen = false;
volatile bool pendingYellow = false;
volatile bool pendingRed = false;

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
// 5. 顏色
// =========================
const uint16_t BG    = 0xFFFF;
const uint16_t PANEL = 0x2145;
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
// 8. 主畫面
// =========================
void drawScreen() {
  tft.fillScreen(BG);

  drawStatusBar();

  tft.setTextColor(0x10A2);
  tft.setTextSize(2);
  tft.setCursor(13, 30);
  tft.print("ENV");

  tft.setTextSize(1);
  tft.setCursor(125, 34);
  tft.print("DHT11");

  // 溫度：49~99
  tft.fillRoundRect(9, 49, 158, 47, 6, PANEL);
  drawThermometer(31, 71);

  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.setCursor(53, 57);
  tft.print("TEMPERATURE");

  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(55, 68);
  if (sensorOK) tft.print(tempC);
  else tft.print("--");

  tft.setTextSize(2);
  tft.setCursor(119, 74);
  tft.setTextColor(GREEN);
  tft.print("C");

  // 濕度：102~149
  tft.fillRoundRect(9, 102, 158, 47, 6, PANEL);
  drawDrop(31, 125);

  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.setCursor(53, 110);
  tft.print("HUMIDITY");

  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(55, 121);
  if (sensorOK) tft.print(hum);
  else tft.print("--");

  tft.setTextSize(2);
  tft.setCursor(119, 127);
  tft.setTextColor(GREEN);
  tft.print("%");

  // 光線：155~211，不再被底部訊息覆蓋
  tft.fillRoundRect(9, 155, 158, 57, 6, PANEL);
  drawSun(31, 184);

  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.setCursor(53, 164);
  tft.print("LIGHT");

  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(55, 177);
  tft.print(light);

  tft.setTextSize(2);
  tft.setCursor(119, 183);
  tft.setTextColor(GREEN);
  tft.print("%");
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
    bool subGreen  = mqttClient.subscribe(MQTT_GREEN_TOPIC);
    bool subYellow = mqttClient.subscribe(MQTT_YELLOW_TOPIC);
    bool subRed    = mqttClient.subscribe(MQTT_RED_TOPIC);

    Serial.print("Subscribe green: "); Serial.println(subGreen ? "OK" : "FAIL");
    Serial.print("Subscribe yellow: "); Serial.println(subYellow ? "OK" : "FAIL");
    Serial.print("Subscribe red: "); Serial.println(subRed ? "OK" : "FAIL");

    mqttMessage = "OK";
    screenMessage = "MQTT CONNECTED";
    Serial.println("OK");

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
// MQTT LED 控制（依 27_mqtt_retained）
// =========================
const char* getLedName(const char* topic) {
  if (strcmp(topic, MQTT_GREEN_TOPIC) == 0) return "GLED";
  if (strcmp(topic, MQTT_YELLOW_TOPIC) == 0) return "YLED";
  if (strcmp(topic, MQTT_RED_TOPIC) == 0) return "RLED";
  return nullptr;
}

int getLedPin(const char* topic) {
  if (strcmp(topic, MQTT_GREEN_TOPIC) == 0) return LED_GREEN;
  if (strcmp(topic, MQTT_YELLOW_TOPIC) == 0) return LED_YELLOW;
  if (strcmp(topic, MQTT_RED_TOPIC) == 0) return LED_RED;
  return -1;
}

// 找 JSON 中指定 LED 欄位的 on/off。
// 支援：{"gled":"on"}、{"gled": "off"} 等常見空白格式。
bool jsonLedState(const char* message, const char* key, bool &state) {
  const char* p = strstr(message, key);
  if (p == nullptr) return false;

  p = strchr(p, ':');
  if (p == nullptr) return false;
  p++;

  while (*p == ' ' || *p == '\t' || *p == '"' ) {
    if (*p == '"') { p++; break; }
    p++;
  }

  while (*p == ' ' || *p == '\t' || *p == '"') p++;

  if (strncasecmp(p, "on", 2) == 0 || strncmp(p, "1", 1) == 0 ||
      strncasecmp(p, "true", 4) == 0) {
    state = true;
    return true;
  }

  if (strncasecmp(p, "off", 3) == 0 || strncmp(p, "0", 1) == 0 ||
      strncasecmp(p, "false", 5) == 0) {
    state = false;
    return true;
  }

  return false;
}

// MQTT callback：只做「解析 + GPIO」，不要在這裡畫 TFT 或 publish MQTT。
// GPIO 會優先切換，讓 LED 反應最快。
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char message[64];
  unsigned int copyLength = min(length, (unsigned int)(sizeof(message) - 1));
  memcpy(message, payload, copyLength);
  message[copyLength] = '\0';

  int ledPin = getLedPin(topic);
  if (ledPin < 0) return;

  bool ledOn = false;
  bool valid = false;
  bool isJsonCommand = strchr(message, '{') != nullptr;

  if (isJsonCommand) {
    if (ledPin == LED_GREEN) {
      valid = jsonLedState(message, "\"gled\"", ledOn);
    } else if (ledPin == LED_YELLOW) {
      valid = jsonLedState(message, "\"yled\"", ledOn);
    } else if (ledPin == LED_RED) {
      valid = jsonLedState(message, "\"rled\"", ledOn);
    }
  } else {
    // 保留 27 的 ON/OFF 控制格式
    if (strcasecmp(message, "ON") == 0) {
      ledOn = true;
      valid = true;
    } else if (strcasecmp(message, "OFF") == 0) {
      ledOn = false;
      valid = true;
    }
  }

  if (!valid) return;

  // 最重要：先切 GPIO，不能讓 TFT 或 MQTT publish 延遲 LED。
  digitalWrite(ledPin, ledOn ? HIGH : LOW);

  // JSON 指令需要保留最後狀態，但延後到 loop() 做。
  if (isJsonCommand) {
    if (ledPin == LED_GREEN) {
      pendingGreen = true;
      pendingGreenState = ledOn;
    } else if (ledPin == LED_YELLOW) {
      pendingYellow = true;
      pendingYellowState = ledOn;
    } else if (ledPin == LED_RED) {
      pendingRed = true;
      pendingRedState = ledOn;
    }
  }

  Serial.print("LED: ");
  Serial.print(getLedName(topic));
  Serial.print(" -> ");
  Serial.println(ledOn ? "ON" : "OFF");
}

// 把 retained 狀態放到 callback 外處理，避免阻塞 MQTT 收包。
void processPendingLedStates() {
  if (!mqttClient.connected()) return;

  if (pendingGreen) {
    bool state = pendingGreenState;
    pendingGreen = false;
    mqttClient.publish(MQTT_GREEN_TOPIC, state ? "ON" : "OFF", true);
  }

  if (pendingYellow) {
    bool state = pendingYellowState;
    pendingYellow = false;
    mqttClient.publish(MQTT_YELLOW_TOPIC, state ? "ON" : "OFF", true);
  }

  if (pendingRed) {
    bool state = pendingRedState;
    pendingRed = false;
    mqttClient.publish(MQTT_RED_TOPIC, state ? "ON" : "OFF", true);
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

  // 初始化 TFT
  tft.begin();

  // 先顯示啟動畫面
  drawScreen();

  // 先讀一次感測器
  readSensors();

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
void loop() {
  unsigned long now = millis();

  // =====================================================
  // MQTT 放在 loop 最前面：優先處理 LED 指令，降低延遲
  // =====================================================
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      mqttMessage = "X";

      // 每 5 秒重新連 MQTT
      if (now - lastMQTTAttempt >= 5000UL) {
        lastMQTTAttempt = now;
        connectMQTT();
      }
    } else {
      mqttMessage = "OK";

      // 持續處理 MQTT 封包；LED callback 會立即切 GPIO
      mqttClient.loop();

      // retained 狀態在 callback 完成後再送
      processPendingLedStates();
    }
  }

  // -------------------------
  // WiFi 狀態
  // -------------------------
  if (WiFi.status() != WL_CONNECTED) {
    wifiMessage = "X";
    mqttMessage = "X";

    static unsigned long lastWiFiAttempt = 0;

    if (now - lastWiFiAttempt >= 5000UL) {
      lastWiFiAttempt = now;
      connectWiFi();

      if (WiFi.status() == WL_CONNECTED) {
        mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
        connectMQTT();
      }
    }
  } else {
    wifiMessage = "OK";
  }

  // -------------------------
  // 每 5 秒讀取感測器
  // -------------------------
  if (now - lastRead >= SENSOR_INTERVAL) {
    lastRead = now;

    readSensors();
    drawScreen();
  }

  // -------------------------
  // 每 10 秒送 MQTT JSON
  // -------------------------
  if (now - lastMQTTPublish >= MQTT_INTERVAL) {
    lastMQTTPublish = now;

    if (WiFi.status() == WL_CONNECTED) {
      if (!mqttClient.connected()) {
        connectMQTT();
      }

      if (mqttClient.connected()) {
        publishData();
      }
    }
  }

  // 不在這裡每秒重畫 TFT。
  // TFT 重畫會暫時占用 CPU/SPI，會影響 MQTT LED 指令延遲。
  // 連線狀態只在連線事件發生時更新。
  yield();
}


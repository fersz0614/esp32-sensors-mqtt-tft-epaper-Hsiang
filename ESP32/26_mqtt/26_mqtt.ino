#include <SimpleDHT.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <PubSubClient.h>

constexpr uint8_t DHT11_PIN = 14;
constexpr uint8_t LIGHT_PIN = 33;
constexpr uint8_t LED_GREEN = 15;
constexpr uint8_t LED_YELLOW = 2;
constexpr uint8_t LED_RED = 4;

constexpr char WIFI_SSID[] = "a";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char MQTT_HOST[] = "mqttgo.io";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_TOPIC[] = "Hsiang/class305/data";
constexpr unsigned long SENSOR_INTERVAL = 1000;
constexpr unsigned long MQTT_INTERVAL = 10000;

SimpleDHT11 dht11(DHT11_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

byte temperature = 0;
byte humidity = 0;
int lightPercent = 0;
int dhtError = SimpleDHTErrSuccess;
unsigned long lastSensorRead = 0;
unsigned long lastMqttPublish = 0;
String mqttStatus = "MQTT...";

void drawThermometer(uint8_t x, uint8_t y) {
  u8g2.drawFrame(x + 3, y, 5, 11);
  u8g2.drawDisc(x + 5, y + 13, 4);
  u8g2.drawLine(x + 5, y + 3, x + 5, y + 12);
}

void drawDrop(uint8_t x, uint8_t y) {
  u8g2.drawCircle(x + 5, y + 9, 4);
  u8g2.drawTriangle(x + 5, y, x + 1, y + 8, x + 9, y + 8);
}

void drawSun(uint8_t x, uint8_t y) {
  u8g2.drawCircle(x + 6, y + 6, 3);
  u8g2.drawLine(x + 6, y, x + 6, y + 2);
  u8g2.drawLine(x + 6, y + 10, x + 6, y + 12);
  u8g2.drawLine(x, y + 6, x + 2, y + 6);
  u8g2.drawLine(x + 10, y + 6, x + 12, y + 6);
  u8g2.drawLine(x + 1, y + 1, x + 3, y + 3);
  u8g2.drawLine(x + 9, y + 9, x + 11, y + 11);
  u8g2.drawLine(x + 9, y + 3, x + 11, y + 1);
  u8g2.drawLine(x + 1, y + 11, x + 3, y + 9);
}

void updateLeds() {
  bool warning = dhtError == SimpleDHTErrSuccess && (temperature > 28 || humidity > 70);
  bool error = dhtError != SimpleDHTErrSuccess || (WiFi.status() == WL_CONNECTED && !mqttClient.connected());
  digitalWrite(LED_GREEN, !warning && !error);
  digitalWrite(LED_YELLOW, warning && !error);
  digitalWrite(LED_RED, error);
}

void drawDashboard() {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 0, WiFi.status() == WL_CONNECTED ? "ONLINE" : "OFFLINE");
  // Complete rounded frame enclosing every icon and value label.
  u8g2.drawRFrame(0, 9, 128, 45, 3);
  drawThermometer(11, 16);
  drawDrop(53, 16);
  drawSun(98, 16);

  char line[24];
  if (dhtError == SimpleDHTErrSuccess) {
    snprintf(line, sizeof(line), "T:%uC", temperature);
    u8g2.drawStr(5, 40, line);
    snprintf(line, sizeof(line), "H:%u%%", humidity);
    u8g2.drawStr(47, 40, line);
  } else {
    u8g2.drawStr(5, 40, "DHT err");
  }
  snprintf(line, sizeof(line), "L:%d%%", lightPercent);
  u8g2.drawStr(89, 40, line);
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(2, 55, mqttStatus.c_str());
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.sendBuffer();
}

void readSensors() {
  dhtError = dht11.read(&temperature, &humidity, nullptr);
  int lightRaw = analogRead(LIGHT_PIN);
  lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);
  updateLeds();
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (uint8_t attempt = 0; attempt < 40 && WiFi.status() != WL_CONNECTED; ++attempt) delay(500);
  return WiFi.status() == WL_CONNECTED;
}

bool connectMqtt() {
  if (mqttClient.connected()) return true;
  uint32_t randomPart = esp_random();
  char clientId[32];
  snprintf(clientId, sizeof(clientId), "esp32-%08lX", (unsigned long)randomPart);
  mqttStatus = "MQTT connect";
  drawDashboard();
  bool ok = mqttClient.connect(clientId);
  mqttStatus = ok ? "MQTT ready" : "MQTT error";
  updateLeds();
  drawDashboard();
  return ok;
}

bool publishData() {
  if (!connectMqtt() || dhtError != SimpleDHTErrSuccess) return false;
  char payload[64];
  snprintf(payload, sizeof(payload), "{\"temp\":%u,\"humi\":%u,\"light\":%d}", temperature, humidity, lightPercent);
  bool ok = mqttClient.publish(MQTT_TOPIC, payload);
  mqttStatus = ok ? "MQTT sent" : "MQTT error";
  updateLeds();
  drawDashboard();
  Serial.printf("MQTT %s: %s\n", ok ? "published" : "publish failed", payload);
  return ok;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(LIGHT_PIN, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, HIGH);

  Wire.begin(21, 22);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setFontPosTop();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectWiFi();
  readSensors();
  connectMqtt();
  drawDashboard();
  lastSensorRead = millis();
  lastMqttPublish = millis();
}

void loop() {
  unsigned long now = millis();
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    readSensors();
    drawDashboard();
  }
  if (!mqttClient.connected()) connectMqtt();
  mqttClient.loop();
  if (now - lastMqttPublish >= MQTT_INTERVAL) {
    lastMqttPublish = now;
    publishData();
  }
}

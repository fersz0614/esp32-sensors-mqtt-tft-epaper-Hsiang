#include <SimpleDHT.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

constexpr uint8_t DHT11_PIN = 14;
constexpr uint8_t LIGHT_PIN = 33;
constexpr char WIFI_SSID[] = "a";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char THINGSPEAK_KEY[] = "YOUR_THINGSPEAK_WRITE_KEY";
constexpr unsigned long UPLOAD_INTERVAL = 15000;

SimpleDHT11 dht11(DHT11_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0, /* reset=*/U8X8_PIN_NONE);

byte temperature = 0;
byte humidity = 0;
int lightPercent = 0;
int dhtError = SimpleDHTErrSuccess;
String uploadStatus = "Starting";
unsigned long lastSensorRead = 0;
unsigned long lastUpload = 0;

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

void showMessage(const String &message) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 0, "WiFi / ThingSpeak");
  u8g2.drawStr(0, 22, message.c_str());
  u8g2.sendBuffer();
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  showMessage("Connecting WiFi");

  for (uint8_t attempt = 0; attempt < 40 && WiFi.status() != WL_CONNECTED;
       ++attempt) {
    delay(500);
    u8g2.setCursor((attempt % 18) * 6, 40);
    u8g2.print('.');
    u8g2.sendBuffer();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    showMessage("WiFi connected!");
    delay(1000);
    return true;
  }

  Serial.println("WiFi connection failed");
  showMessage("WiFi failed");
  delay(1000);
  return false;
}

void readSensors() {
  dhtError = dht11.read(&temperature, &humidity, nullptr);
  int lightRaw = analogRead(LIGHT_PIN);
  lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);
}

void drawDashboard() {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 0, WiFi.status() == WL_CONNECTED ? "ONLINE" : "OFFLINE");

  drawThermometer(11, 16);
  drawDrop(53, 16);
  drawSun(95, 16);

  char line[24];
  if (dhtError == SimpleDHTErrSuccess) {
    snprintf(line, sizeof(line), "T:%uC", temperature);
    u8g2.drawStr(5, 40, line);
    snprintf(line, sizeof(line), "H:%u%%", humidity);
    u8g2.drawStr(47, 40, line);
  } else {
    u8g2.drawStr(2, 40, "DHT err");
  }

  snprintf(line, sizeof(line), "L:%d%%", lightPercent);
  u8g2.drawStr(89, 40, line);
  u8g2.drawHLine(0, 54, 128);
  u8g2.drawStr(2, 56, uploadStatus.c_str());
  u8g2.sendBuffer();
}

void uploadToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    uploadStatus = "WiFi reconnect";
    drawDashboard();
    connectWiFi();
    return;
  }

  uploadStatus = "Uploading...";
  drawDashboard();

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = "https://api.thingspeak.com/update?api_key=" +
               String(THINGSPEAK_KEY) + "&field1=" + String(temperature) +
               "&field2=" + String(humidity) + "&field3=" + String(lightPercent);

  if (http.begin(client, url)) {
    int httpCode = http.GET();
    String response = http.getString();
    Serial.printf("ThingSpeak HTTP %d, response: %s\n", httpCode,
                  response.c_str());
    uploadStatus = (httpCode == HTTP_CODE_OK && response != "0") ? "Uploaded OK"
                                                                  : "Upload error";
    http.end();
  } else {
    uploadStatus = "HTTP error";
  }
  drawDashboard();
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(LIGHT_PIN, INPUT);

  Wire.begin(21, 22);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setFontPosTop();

  connectWiFi();
  readSensors();
  drawDashboard();
  lastSensorRead = millis();
  lastUpload = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSensorRead >= 1000) {
    lastSensorRead = now;
    readSensors();
    drawDashboard();
    Serial.printf("Temperature: %u C, Humidity: %u %%, Light: %d %%\n",
                  temperature, humidity, lightPercent);
  }

  if (now - lastUpload >= UPLOAD_INTERVAL) {
    lastUpload = now;
    uploadToThingSpeak();
  }
}

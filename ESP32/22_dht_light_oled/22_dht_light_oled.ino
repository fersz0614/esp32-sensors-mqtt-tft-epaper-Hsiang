#include <SimpleDHT.h>
#include <Wire.h>
#include <U8g2lib.h>

// Sensor pins
constexpr uint8_t DHT11_PIN = 14;
constexpr uint8_t LIGHT_PIN = 33;

SimpleDHT11 dht11(DHT11_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0, /* reset=*/U8X8_PIN_NONE);

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

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // ESP32 ADC: 0..4095
  pinMode(LIGHT_PIN, INPUT);

  Wire.begin(21, 22);         // SDA=GPIO21, SCL=GPIO22
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setFontPosTop();
}

void loop() {
  byte temperature = 0;
  byte humidity = 0;
  int err = dht11.read(&temperature, &humidity, nullptr);

  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);

  u8g2.clearBuffer();
  u8g2.drawStr(0, 0, "ESP32 STATUS");

  // Three separated columns: icon on top, value underneath.
  drawThermometer(11, 16);
  drawDrop(53, 16);
  drawSun(95, 16);

  if (err == SimpleDHTErrSuccess) {
    char line[32];
    snprintf(line, sizeof(line), "T:%uC", temperature);
    u8g2.drawStr(5, 40, line);
    snprintf(line, sizeof(line), "H:%u%%", humidity);
    u8g2.drawStr(47, 40, line);
    snprintf(line, sizeof(line), "L:%d%%", lightPercent);
    u8g2.drawStr(89, 40, line);
  } else {
    u8g2.drawStr(2, 40, "DHT err");
  }

  u8g2.drawHLine(0, 54, 128);
  u8g2.drawStr(2, 56, "TEMP");
  u8g2.drawStr(45, 56, "HUM");
  u8g2.drawStr(86, 56, "LIGHT");
  u8g2.sendBuffer();

  Serial.printf("Temperature: %u C, Humidity: %u %%, Light: %d/4095 (%d %%)\n",
                temperature, humidity, lightRaw, lightPercent);
  delay(1000);
}

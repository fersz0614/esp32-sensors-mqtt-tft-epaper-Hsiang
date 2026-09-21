// DHT宣告
#include <SimpleDHT.h>
int pinDHT11 = 19;
SimpleDHT11 dht11(pinDHT11);

// 蜂鳴器宣告
#include <ESP32Servo.h>
int buzzer = 17;

// OLED+RGBLED宣告
#include <Wire.h>
#include <U8g2lib.h>  // OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

int R = 25;
int G = 26;
int B = 27;

void setup() {
  Serial.begin(115200);
  u8g2.begin();                                // 初始化
  u8g2.enableUTF8Print();                      // 啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  // 設定使用中文字形
  u8g2.setFontPosTop();                        // 座標從上開始

  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(buzzer, OUTPUT);
}

void loop() {
  Serial.println("=================================");
  Serial.println("Sample DHT11...");

  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;

  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err=");
    Serial.print(SimpleDHTErrCode(err));
    Serial.print(",");
    Serial.println(SimpleDHTErrDuration(err));
    delay(1000);
    return;
  }

  Serial.println("Sample OK: " + (String)temperature + " *C, " + (String)humidity + " H");

  // RGB LED 顯示濕度狀態
  if (humidity <= 60) { analogWrite(R, 0); analogWrite(G, 255); analogWrite(B, 0); }
  if (humidity >= 61 && humidity <= 70) { analogWrite(R, 255); analogWrite(G, 255); analogWrite(B, 0); }
  if (humidity >= 71 && humidity <= 80) { analogWrite(R, 255); analogWrite(G, 187); analogWrite(B, 0); }
  if (humidity >= 81 && humidity <= 90) { analogWrite(R, 0); analogWrite(G, 255); analogWrite(B, 204); }
  if (humidity >= 91 && humidity <= 100) { analogWrite(R, 255); analogWrite(G, 0); analogWrite(B, 0); }

  // 蜂鳴器警示
  if (humidity >= 71) {
    tone(buzzer, 262, 500);
    delay(600);
    tone(buzzer, 294, 500);
    delay(600);
    tone(buzzer, 330, 500);
    delay(600);
    noTone(buzzer);
  }

  // OLED 顯示
  u8g2.clearBuffer();
  u8g2.setCursor(0, 5);
  u8g2.print("高屏澎勞動部分署");

  u8g2.setCursor(0, 25);
  u8g2.print("溫度：" + (String)temperature + " C");

  u8g2.setCursor(0, 45);
  u8g2.print("濕度：" + (String)humidity + " %");

  u8g2.sendBuffer();
  delay(1000);
}

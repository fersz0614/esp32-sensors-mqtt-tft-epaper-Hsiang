//DHT宣告
#include <SimpleDHT.h>
int pinDHT11 = 19;
SimpleDHT11 dht11(pinDHT11);

//OLED宣告
#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

//LED宣告
int G = 15;
int R = 0;
int Y = 2;
int B = 4;

void setup() {
  Serial.begin(115200);
  u8g2.begin();                                
  u8g2.enableUTF8Print();                      
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  
  u8g2.setFontPosTop();
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(Y, OUTPUT);
  pinMode(B, OUTPUT);
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
  
  Serial.println("Sample OK: " + String(temperature) +" *C, " + String(humidity) +" H");

  if(humidity > 70){ digitalWrite(R,HIGH); digitalWrite(G,LOW); }
  else{ digitalWrite(G,HIGH); digitalWrite(R,LOW); }

  if(temperature > 26){ digitalWrite(Y,HIGH); digitalWrite(B,LOW); }
  else{ digitalWrite(B,HIGH); digitalWrite(Y,LOW); }

  // 顯示 OLED
  u8g2.clearBuffer();  

  if (humidity > 70 || temperature > 28) {
    // 警告模式：整個畫面顯示警告
    u8g2.setCursor(0, 5);
    u8g2.println("高屏澎勞動部分署");
    if (humidity > 70) {
      u8g2.setCursor(0, 25);
      u8g2.println(" !!濕度異常!!");
    }
    if (temperature > 28) {
      u8g2.setCursor(0, 35);
      u8g2.println(" !!溫度異常!!");
    }
  } else {
    // 正常模式：顯示溫溼度
    u8g2.setCursor(0, 5);
    u8g2.print("高屏澎勞動部分署");

    // 畫溫度計圖示
    u8g2.drawFrame(0, 25, 8, 20);   // 溫度計外框
    u8g2.drawBox(2, 35, 4, 8);      // 溫度計底部
    u8g2.drawBox(2, 25, 4, (temperature/2)); // 溫度柱狀顯示

    u8g2.setCursor(12, 25);
    u8g2.print("溫度：" + String(temperature) + " C");
    // 畫水滴圖示
    u8g2.drawCircle(4, 50, 4, U8G2_DRAW_ALL); // 水滴圓形
    u8g2.drawTriangle(0, 50, 8, 50, 4, 42);   // 水滴尖端

    u8g2.setCursor(12, 45);
    u8g2.print("濕度：" + String(humidity) + " %");
    u8g2.setCursor(0, 45);
  }

  u8g2.sendBuffer();
  delay(5000);
}
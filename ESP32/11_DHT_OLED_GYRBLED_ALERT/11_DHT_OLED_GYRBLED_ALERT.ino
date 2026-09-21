
//DHT宣告
#include <SimpleDHT.h>
int pinDHT11 = 19;
SimpleDHT11 dht11(pinDHT11);

//OLED宣告
#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
int G = 15;
int Y = 2;
int R = 0;
int B = 4;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  u8g2.begin();                                //初始化
  u8g2.enableUTF8Print();                      //啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  u8g2.setFontPosTop();                        //座標從上開始
  pinMode(G, OUTPUT);
  pinMode(Y, OUTPUT);
  pinMode(R, OUTPUT);
  pinMode(B, OUTPUT);
}

void loop() {
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  // put your main code here, to run repeatedly:
  //讀取溫溼度
  Serial.println("=================================");
  Serial.println("Sample DHT11...");

  //byte=>0~255範圍的整數 ,int=>20億左右, long 天文
  byte temperature = 0;
  byte humidity = 0;
  //如果讀取錯誤,就印出錯誤訊息並返回return,返回開始的地方
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

  if (humidity > 70) {
    digitalWrite(R, HIGH);
    digitalWrite(G, LOW);

    u8g2.clearBuffer();              //顯示前清除螢幕
    u8g2.setCursor(0, 5);            //移動游標
    u8g2.print("高屏澎勞動部分屬");  //寫入文字

    u8g2.setCursor(0, 35);         //移動游標
    u8g2.print("  !!濕度異常!!");  //寫入文字

    //u8g2.drawLine(0, 11, 30, 11);  //劃線從0,11->30,11

    u8g2.sendBuffer();  //送到螢幕顯示
    delay(3000);
  } else {
    digitalWrite(R, LOW);
    digitalWrite(G, HIGH);
  }

  if (temperature > 26) {
    digitalWrite(Y, HIGH);
    digitalWrite(B, LOW);
    u8g2.clearBuffer();              //顯示前清除螢幕
    u8g2.setCursor(0, 5);            //移動游標
    u8g2.print("高屏澎勞動部分屬");  //寫入文字

    u8g2.setCursor(0, 35);         //移動游標
    u8g2.print("  !!溫度異常!!");  //寫入文字

    //u8g2.drawLine(0, 11, 30, 11);  //劃線從0,11->30,11

    u8g2.sendBuffer();  //送到螢幕顯示
    delay(3000);
  } else {
    digitalWrite(Y, LOW);
    digitalWrite(B, HIGH);
  }


  u8g2.clearBuffer();              //顯示前清除螢幕
  u8g2.setCursor(0, 5);            //移動游標
  u8g2.print("高屏澎勞動部分屬");  //寫入文字

  u8g2.setCursor(0, 25);  //移動游標

  u8g2.print("   溫度：" + (String)temperature + " C");  //寫入文字

  u8g2.setCursor(0, 45);                           //移動游標
  u8g2.print("濕度：" + (String)humidity + " %");  //寫入文字

  u8g2.sendBuffer();  //送到螢幕顯示
  delay(2000);
}

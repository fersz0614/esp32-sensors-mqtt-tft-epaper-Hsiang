//OLED宣告
#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

int Trig = 12;   // 發出聲波腳位
int Echo = 14;   // 接收聲波腳位
int Buzzer = 17; // 蜂鳴器腳位

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  u8g2.begin();                                //初始化
  u8g2.enableUTF8Print();                      //啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  u8g2.setFontPosTop();//座標從上開始
  pinMode(Trig, OUTPUT);pinMode(Echo, INPUT);pinMode(Buzzer, OUTPUT);
}

void loop() {
  // 超音波測距
  digitalWrite(Trig, LOW); 
  delayMicroseconds(5);
  digitalWrite(Trig, HIGH);
  delayMicroseconds(10);  
  digitalWrite(Trig, LOW);

  float EchoTime = pulseIn(Echo, HIGH); 
  float CMValue = EchoTime / 29.4 / 2; 
  Serial.println(CMValue);

  // 蜂鳴器警報
  if (CMValue <= 10) {
    tone(Buzzer, 1000); // 在 GPIO17 蜂鳴器發出 1000Hz 警報
  } else {
    noTone(Buzzer);     // 停止警報
  }

 // OLED 顯示距離
  u8g2.clearBuffer();                        //顯示前清除螢幕
  u8g2.setCursor(0, 7);                     //移動游標
  u8g2.print("高屏澎勞動部分署");           //寫入文字

  u8g2.setCursor(0, 35);                     //移動游標
  u8g2.print("距離: ");           //寫入文字
  u8g2.print(CMValue, 1); // 顯示一位小數
  u8g2.print(" cm");
  //u8g2.drawLine(0, 11, 30, 11);  //劃線從0,11->30,11

  u8g2.sendBuffer();  //送到螢幕顯示

  delay(50);
}

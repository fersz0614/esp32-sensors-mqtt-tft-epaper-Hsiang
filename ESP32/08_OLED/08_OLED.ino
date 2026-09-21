/*
  縮網址：twgo.io/oleds
  此為OLED 範例程式，須使用U8G2程式庫
  原始U8G2版本程式庫缺乏台灣中文字
  請下載益師父版本（約7000中文字）：twgo.io/vygya
*/
#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
// 0.96吋
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// 1.3吋 (用錯，右側會有一條橫線)
//U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

void setup() {
  u8g2.begin();                                //初始化
  u8g2.enableUTF8Print();                      //啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  u8g2.setFontPosTop();//座標從上開始
  //u8g2.setFontDirection(0);//0不旋轉、1->90、2->180、3->270
  //u8g2.setFlipMode(1);//上下相反

}
int i = 0;
void loop() {
  u8g2.clearBuffer();                        //顯示前清除螢幕
  u8g2.setCursor(0, 5);                     //移動游標
  u8g2.print(" 屏東科技大學");           //寫入文字

  u8g2.setCursor(0, 25);                     //移動游標
  u8g2.print("溫濕度：123456789");           //寫入文字

  u8g2.setCursor(0, 45);                    //移動游標
  u8g2.print("目前已啟動：" + String(i++));  //寫入文字

  u8g2.drawLine(0, 11, 30, 11);  //劃線從0,11->30,11

  u8g2.sendBuffer();  //送到螢幕顯示
  delay(1000);
}

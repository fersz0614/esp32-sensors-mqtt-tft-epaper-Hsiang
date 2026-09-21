void setup() {
  // 初始化，只執行一次
Serial.begin(115200);//序列啟動,15200速率(秒/b),功能為送出除錯訊息
}

void loop() {
  // 重複執行，無止無盡
Serial.println("哈囉");
delay(500);
Serial.println("老師,好像要吃飯了喔");
delay(500);
Serial.println("跟世界脫節");
delay(500);//1000ms=1s秒
}

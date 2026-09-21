// 腳位設定
int redPin = 0;
int yellowPin = 2;
int greenPin = 15;

void setup() {
  // 設定三個 LED 腳位為輸出
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
}

void loop() {
  // 綠燈亮 5 秒
  digitalWrite(greenPin, HIGH);
  delay(5000);
  digitalWrite(greenPin, LOW);

  // 黃燈亮 1 秒
  digitalWrite(yellowPin, HIGH);
  delay(1000);
  digitalWrite(yellowPin, LOW);

  // 紅燈亮 3 秒
  digitalWrite(redPin, HIGH);
  delay(3000);
  digitalWrite(redPin, LOW);
}

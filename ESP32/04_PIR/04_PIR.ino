// HC-SR501 人體感測器 + LED 控制
// OUT 接 ESP32 GPIO18
// 紅燈接 GPIO0，綠燈接 GPIO2

const int pirPin = 18;   // 人體感測器腳位
const int redPin = 0;    // 紅燈腳位
const int greenPin = 2;  // 綠燈腳位

void setup() {
  Serial.begin(115200);
  pinMode(pirPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);

  Serial.println("HC-SR501 人體感測器啟動");
  Serial.println("等待感測器穩定中...");
  delay(10000);  // 感測器剛上電需要穩定時間
  Serial.println("開始偵測");
}

void loop() {
  int value = digitalRead(pirPin);

  if (value == HIGH) {
    // 偵測到有人活動 → 紅燈亮 5 秒
    Serial.println("【偵測到人體移動】");
    digitalWrite(redPin, HIGH);
    digitalWrite(greenPin, LOW);
    delay(5000);

    // 5 秒後 → 紅燈關閉，綠燈亮
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, HIGH);
  } else {
    // 沒有人活動 → 綠燈持續亮
    Serial.println("【沒有偵測到人體】");
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, HIGH);
    delay(500); // 每 0.5 秒更新一次狀態
  }
}

// 腳位設定
int sensorPin = 23;   // 光敏模組
int ledPin1 = 15;     // LED1
int ledPin2 = 2;      // LED2
int ledPin3 = 4;      // LED3

void setup() {
  Serial.begin(115200);

  // 設定腳位模式
  pinMode(sensorPin, INPUT);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
}

void loop() {
  // 讀取光敏模組的數位值
  int sensorValue = digitalRead(sensorPin);

  // 輸出到序列監控
  Serial.print("Sensor Value: ");
  Serial.println(sensorValue);

  if (sensorValue == 1) {
    // 光線微弱 → 三顆 LED 全亮
    digitalWrite(ledPin1, HIGH);
    digitalWrite(ledPin2, HIGH);
    digitalWrite(ledPin3, HIGH);
  } else {
    // 光線充足 → 三顆 LED 全滅
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, LOW);
  }

  delay(500); // 每 0.5 秒更新一次
}

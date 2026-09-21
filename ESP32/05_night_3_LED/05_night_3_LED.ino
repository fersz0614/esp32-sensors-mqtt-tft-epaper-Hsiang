const int sensorPin = 36;   // 光敏感測器 AO 腳
const int redPin    = 0;    // 紅燈
const int yellowPin = 2;    // 黃燈
const int greenPin  = 15;   // 綠燈

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // 確保解析度是 0–4095
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
}

void loop() {
  int rawValue = analogRead(sensorPin); 
  int mappedValue = map(rawValue, 0, 4095, 100, 0); // 轉換成 0–100，亮度越高數字越大

  // 只印出數字
  Serial.println(mappedValue);

  // 控制燈號
  if (mappedValue <= 20) {
    digitalWrite(redPin, HIGH);
    digitalWrite(yellowPin, HIGH);
    digitalWrite(greenPin, HIGH);
  } else if (mappedValue <= 40) {
    digitalWrite(redPin, HIGH);
    digitalWrite(yellowPin, HIGH);
    digitalWrite(greenPin, LOW);
  } else if (mappedValue <= 60) {
    digitalWrite(redPin, HIGH);
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin, LOW);
  } else {
    digitalWrite(redPin, LOW);
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin, LOW);
  }

  delay(100); // 每 0.1 秒更新一次
}

int Trig =12;//發出聲波腳位
int Echo =14;//接收聲波腳位
void setup(){
  Serial.begin(115200);
  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT);
}

void loop() {
  digitalWrite(Trig, LOW); //先關閉
  delayMicroseconds(5);
  digitalWrite(Trig, HIGH);//啟動超音波
  delayMicroseconds(10);  
  digitalWrite(Trig, LOW); //關閉
  float EchoTime = pulseIn(Echo, HIGH); //計算傳回時間
  float CMValue = EchoTime / 29.4 / 2; //將時間轉換成距離
  Serial.println(CMValue);
  delay(50);
}

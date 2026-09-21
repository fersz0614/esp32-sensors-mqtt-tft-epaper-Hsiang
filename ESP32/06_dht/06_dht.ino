#include <SimpleDHT.h> //包含 溫溼度設定

// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int pinDHT11 = 19;
//我有一個整數,我把它命名為pinDHT11,預設值為19
SimpleDHT11 dht11(pinDHT11);
//我有一隻SimpleDHT11規格溫濕度計，我把它命名為dht11，(放在19腳)
int G = 15;
int R = 0;
int Y = 2;
int B = 4;

void setup() {
  Serial.begin(115200);
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(Y, OUTPUT);
  pinMode(B, OUTPUT);
}

void loop() {
  // start working...
  Serial.println("=================================");
  Serial.println("Sample DHT11...");
  
  // read without samples.byte =>0~255整數,int=>20億左右,long 天文
  byte temperature = 0;
  byte humidity = 0;
  //如果讀取錯誤,就印出錯誤訊息並返回return,返回開始地方
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    return;
  }
  
 Serial.println("Sample OK: " + (String)temperature +" *C, "+ (String)humidity +" H");
   //Serial.print("Sample OK: ");
   //Serial.print((int)temperature); Serial.print(" *C, "); 
   //Serial.print((int)humidity); Serial.println(" H");
   // (int)強制轉型為整數,
   // DHT11 sampling rate is 1HZ.
   if(humidity>70){digitalWrite(R,HIGH);digitalWrite(G,LOW);}
   else{digitalWrite(G,HIGH);digitalWrite(R,LOW);}
   if(temperature>28){digitalWrite(Y,HIGH);digitalWrite(B,LOW);}
   else{digitalWrite(B,HIGH);digitalWrite(Y,LOW);}
 delay(1500);
}

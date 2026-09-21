#include <ESP32Servo.h>
int Buzzer= 17;
void setup() {
 Serial.begin(115200);
}
void loop() {
 tone(Buzzer, 262, 1000); // C(Do)
 delay(1000);//配合5秒時間
 tone(Buzzer, 294, 1000); // D(Re)
 delay(1000);//配合5秒時間
 tone(Buzzer, 330, 1000); // E(Mi)
 delay(1000);//配合5秒時間
 noTone(Buzzer);// 靜止出聲
 delay(5000);
}
int G=15;

void setup() {
  // put your setup code here, to run once:
pinMode(G,OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
analogWrite(G,13);//255=>100% 0~255 5%=>13
delay (1000);
analogWrite(G,77);//30%
delay (1000);
analogWrite(G,154);//60%
delay (1000);
analogWrite(G,231);//90%
delay (1000);
analogWrite(G,255);//100%
delay (5000);

}

#include <SoftwareSerial.h>

SoftwareSerial esp01(2, 3); // RX, TX

void setup() {
  Serial.begin(9600);
  esp01.begin(9600);
}

void loop() {
  if(Serial.available()) esp01.print((char)Serial.read());
  if(esp01.available()) Serial.print((char)esp01.read());
}
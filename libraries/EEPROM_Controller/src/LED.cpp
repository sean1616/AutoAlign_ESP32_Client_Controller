#include "LED.h"
 #include "Arduino.h"
 #include <EEPROM.h>

 LED::LED(byte p,bool state):pin(p)
 {
    pinMode(pin,OUTPUT);
    digitalWrite(pin,state);
 }
 LED::~LED()
 {
    disattach();
 }

 void LED::on()
 {
    digitalWrite(pin,HIGH);
 }
void LED::off()
{
    digitalWrite(pin,LOW);
}
bool LED::getState()
{
    return digitalRead(pin);
}
void LED::disattach()
{
    digitalWrite(pin,LOW);
    pinMode(pin,INPUT);
}

void LED::EE_Begin()
{
  //宣告使用EEPROM 512 個位置
//   EEPROM.begin(512);
}  

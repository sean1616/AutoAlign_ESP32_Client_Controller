#ifndef _LED_H__
 #define _LED_H__

 //匯入arduino核心標頭檔案
 #include "Arduino.h"
  #include <EEPROM.h>

 class LED
 {
    private:
      byte pin; //控制led使用的引腳

    public:
      LED(byte p,bool state=LOW); //建構函式
      ~LED();                     //解構函式
      byte getPin();              //獲取控制的引腳
      void on();                  //開啟led
      void off();                 //關閉led
      boolean getState();         //獲取led的狀態
      void disattach();           //釋放引腳與led的繫結
      void EE_Begin();
 };

#endif
/*
  The MIT License (MIT)

  Copyright (c) 2021 Travis Smith, Sensorium Embedded, LLC

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//TODO:  
//    Turn all void functions (Start, Stop, & Stream) into bools and block/verify response.
//    enforce MAX_NETWORKS and/or preserve NumNetworks from Scan() and enforce that
//    Add reset pin/control so Teensy can reset the ESP32 and VS1053
//    require term char for all commands (not just c and w)

#ifndef esp32RadioControl_h
#define esp32RadioControl_h

#include "Arduino.h"

#define SERIAL_INPUT_TIMEOUT_MS  50 //mS to wait for each char when expecting data
#define MAX_SSID_PW_LEN         100 //max length of Wifi SSID plus Password 
#define MAX_NETWORKS             50 //max number of networs to read back in Scan()
#define INT_RAD_VERSION      "0.80" //ESP32 Internet Radio FW version expected 

class esp32RadioControl
{
   public:
      esp32RadioControl();
      bool begin(HardwareSerial &port);
      uint16_t Scan();
      bool WiFiConnect(const char *SSID, const char *PW);
      void Start();
      void Stop();
      void Stream(const char * URL);
      String SSID(uint16_t NetworkNum);
      int RSSI(uint16_t NetworkNum);
      
   private:
      bool GetPassFail();
      bool SerialAvailabeTimeout(uint32_t MaxTime);
      bool GetSerialString(char * cpBuf, int iMaxLength);
      HardwareSerial *_port;
      String ssid[MAX_NETWORKS];
      int rssi[MAX_NETWORKS];
};

#endif

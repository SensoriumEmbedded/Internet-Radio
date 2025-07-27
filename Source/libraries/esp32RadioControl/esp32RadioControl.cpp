// Please read esp32RadioControl.h for information about the liscence and authors


#include "esp32RadioControl.h"

esp32RadioControl::esp32RadioControl()
{
   //Constructor
   for (uint16_t NetworkNum = 0; NetworkNum < MAX_NETWORKS; NetworkNum++) 
   {
      ssid[NetworkNum] = "n/a";
      rssi[NetworkNum] = 0;
   }
}

bool esp32RadioControl::begin(HardwareSerial &port) 
{
   _port = &port;
   _port->begin(115200);
   
   _port->print("v"); //get "ESP32 Internet Radio" and code version from the ESP32
   char buf[30];
   
   if (!GetSerialString(buf, 30)) return false;
   if(strcmp(buf, "ESP32 Internet Radio") != 0) return false;
   
   if (!GetSerialString(buf, 30)) return false;
   if(strcmp(buf, INT_RAD_VERSION) != 0) return false; //make sure it's the expected version
   
   return true;
}

uint16_t esp32RadioControl::Scan()  //disconnects from wifi & stops streaming (if aplicable)
{
   _port->printf("s");
   if (!SerialAvailabeTimeout(30000)) return 0; //30 sec response timeout

   char buf[100];
   if (!GetSerialString(buf, 100)) return 0;
   uint16_t NumNetworks = atoi(buf);
   //Serial.printf("%d networks found\n", NumNetworks);
   
   if (NumNetworks == 0) return 0;
   for (uint16_t NetworkNum = 0; NetworkNum < NumNetworks; NetworkNum++) 
   {
      if (!GetSerialString(buf, 100)) return 0; //SSID
      ssid[NetworkNum] = buf;

      if (!GetSerialString(buf, 100)) return 0; //RSSI
      rssi[NetworkNum] = atoi(buf);
      
      //Serial.printf("%02d: %d\t%s\n", NetworkNum + 1, rssi[NetworkNum], ssid[NetworkNum].c_str());
   }
   
   return NumNetworks;
}

String esp32RadioControl::SSID(uint16_t NetworkNum)
{
   return ssid[NetworkNum];
}

int esp32RadioControl::RSSI(uint16_t NetworkNum)
{
   return rssi[NetworkNum];
}

bool esp32RadioControl::WiFiConnect(const char *SSID, const char *PW)
{
   _port->printf("w%s/%s\r", SSID, PW);
   if (!SerialAvailabeTimeout(20000)) return false; //20 sec response timeout
   return GetPassFail();
}

void esp32RadioControl::Stream(const char * URL)
{
   _port->printf("c%s\r", URL);
}

void esp32RadioControl::Start()
{
   _port->print("c\r");
}

void esp32RadioControl::Stop()
{
   _port->print("q");
}


//***  Private Functions

bool esp32RadioControl::GetPassFail()
{
   char buf[10];
   if (GetSerialString(buf, 10))
   {
      if(strcmp(buf, "*Pass") == 0) return true;
   }
   return false;
}

bool esp32RadioControl::SerialAvailabeTimeout(uint32_t MaxTime = SERIAL_INPUT_TIMEOUT_MS)
{
  uint32_t StartMillis = millis();
  
  while(!_port->available() && millis() - StartMillis < MaxTime); // timeout loop
  
  return(_port->available());
}

bool esp32RadioControl::GetSerialString(char * cpBuf, int iMaxLength)
{
  int iCharNum = 0;
  
  iMaxLength--; //always leave room for term char
  while (iCharNum < iMaxLength) 
  {
    if (!SerialAvailabeTimeout()) 
    {
      //Serial.println("Serial Timeout!");  
      return(false);
    }
    cpBuf[iCharNum] = _port->read();
    if (cpBuf[iCharNum] == '\r') break; 
    iCharNum++;
  } 
  cpBuf[iCharNum] = 0; //terminate (or force if at max len)
  return(iCharNum < iMaxLength);  
}



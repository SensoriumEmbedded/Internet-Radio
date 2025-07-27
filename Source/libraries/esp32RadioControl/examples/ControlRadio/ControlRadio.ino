

#include "esp32RadioControl.h"
#include "WIFI_info.h"
  
#define ESP_SERIAL Serial4   //serial port connected to the ESP32 radio Serial2

/*
  Example output:
  Scanning for networks...
  
  4 networks found
  01: -49	DIRECT-4D-HP DeskJet 3700 series
  02: -73	NETGEAR27
  03: -91	Frontier0096
  04: -91	TP
  Connecting to NETGEAR27...Succesful!
  Streaming KSPI Stillwater Radio
  Stopping Stream
  Starting same Stream
  Streaming 'Jazz 88' KBEM-FM Minneapolis
  Finished, any key to go again...
*/

esp32RadioControl espRadio;

void setup()
{
   Serial.begin(115200);

   while(!espRadio.begin(ESP_SERIAL)) 
   {
      Serial.println("ESP32 Radio not found!\n");
      delay(1000);
   }
}

void loop()
{
   
   Serial.println("Scanning for networks...");
   uint16_t NumNetworks = espRadio.Scan();
   Serial.printf("%d networks found\n", NumNetworks);
   if (NumNetworks > 0)
   {
      for (uint16_t NetworkNum = 0; NetworkNum < NumNetworks; NetworkNum++) 
      {
         Serial.printf("%02d: %d\t%s\n", NetworkNum + 1, espRadio.RSSI(NetworkNum), espRadio.SSID(NetworkNum).c_str());
      }
   }
   
   Serial.printf("Connecting to %s...", SECRET_SSID);
   if (espRadio.WiFiConnect(SECRET_SSID, SECRET_PASS)) 
   {
      Serial.println("Succesful!");   
   }
   else
   {
      Serial.println("Failed (Halting)");   
      while(1);
   }
   
   Serial.println("Streaming KSPI Stillwater Radio");
   espRadio.Stream("ice24.securenetsystems.net/KSPIFM");  
   delay(15000);
   
   Serial.println("Stopping Stream");
   espRadio.Stop();
   delay(5000);
   
   Serial.println("Starting same Stream");
   espRadio.Start();
   delay(15000);

   Serial.println("Streaming 'Jazz 88' KBEM-FM Minneapolis");
   espRadio.Stream("quarrel.str3am.com:7110/live");  
   
   Serial.println("Finished, any key to go again...\n");
   while(!Serial.available());   
   while(Serial.available())  //flush input buffer
   {
      Serial.read();
      delay(50);
   }
}
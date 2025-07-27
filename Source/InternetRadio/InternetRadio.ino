
#include <SPI.h>
#include <TFT_eSPI.h>
#include <VS1053.h>
#include <WiFi.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <Bounce2.h>
#include <RotaryEncoder.h>
#include <FS.h>
#include <SD.h>
#include <TJpg_Decoder.h>
#include "InternetRadio.h"

uint32_t LastUpdateMs = millis();
uint16_t StationSelect = 0;
uint16_t BytesUntilMetaData = 0;
uint16_t MetaDataInterval = 0;
char TrackName[MAX_TITLE_LEN] = "";  
bool PrintUpdates = false;  //print periodic updates to USB
bool isStreaming = false;  //Are we Streaming currently?
bool MetaDataPresent = false;    //Meta Data detected in Stream
bool ReConnect = false; //force re-connect from main loop
bool ForceRebuf = false; //force rebuffer on under-run
IPAddress ntpServerIP; // NTP server's ip address, resolved from name at WiFi connect

//these are populated from SD card "/NetRadio.txt" file
char ntpServerName[32] = "us.pool.ntp.org";  // NetTimeProtocol Server name
int timeZone = -7;  //Pacific Daylight Time (USA)
char WiFiSSID[MAX_SSID_CHAR_LEN];
char WiFiPassword[MAX_CHAR_INP_LEN];
StructStationInfo RadioStation[MAX_STATIONS] = {"1940s/50s Radio Retro","64.150.176.42",8098,"/"};
uint16_t NumStations = 1;   // # stations stored
uint16_t CurrentStationNum = 0;  //current station
uint16_t DefaultStationNum = 0;  //Default station
StructStationInfo CurrentStation = RadioStation[CurrentStationNum];

WiFiUDP Udp;
TFT_eSPI tft = TFT_eSPI();
VS1053 player(VS1053_CS_PIN, VS1053_DCS_PIN, VS1053_DREQ_PIN);
WiFiClient client; // Use WiFiClient class to create HTTP/TCP connection
Bounce2::Button SelectButton = Bounce2::Button();
Bounce2::Button CancelButton = Bounce2::Button();

//RotaryEncoder Constructor sets INPUT_PULLUPs
#ifdef PHILCO_1941_RADIO_HW
   RotaryEncoder RotEncSel(ROT_ENC_SEL_1_PIN, ROT_ENC_SEL_0_PIN, RotaryEncoder::LatchMode::FOUR3);
   RotaryEncoder RotEncCanc(ROT_ENC_CANC_0_PIN, ROT_ENC_CANC_1_PIN, RotaryEncoder::LatchMode::FOUR3);
#else
   RotaryEncoder RotEncSel(ROT_ENC_SEL_0_PIN, ROT_ENC_SEL_1_PIN, RotaryEncoder::LatchMode::TWO03);
   RotaryEncoder RotEncCanc(ROT_ENC_CANC_0_PIN, ROT_ENC_CANC_1_PIN, RotaryEncoder::LatchMode::TWO03);
#endif

void setup() 
{
  pinMode(BLUE_LED_PIN, OUTPUT);
  digitalWrite(BLUE_LED_PIN, true); //I'm alive!
  Serial.begin(115200);

  Serial.println("\n\nTrav's ESP32/VS1053 Internet Radio");

  pinMode(VS_TFT_RESET_PIN, OUTPUT);
  digitalWrite(VS_TFT_RESET_PIN, false); //Place the VS1053 and TFT display in reset
  //Drive CS LInes of all SPI devices (except TFT disp) high
  pinMode(VS1053_CS_PIN, OUTPUT);
  digitalWrite(VS1053_CS_PIN, true); 
  pinMode(VS1053_DCS_PIN, OUTPUT); 
  digitalWrite(VS1053_DCS_PIN, true); 
  pinMode(SD_CARD_CS_PIN, OUTPUT); 
  digitalWrite(SD_CARD_CS_PIN, true); 
  delay(10);
  digitalWrite(VS_TFT_RESET_PIN, true); //Release reset

  SPI.begin();

  pinMode(TFT_BACKLIGHT_PIN, OUTPUT); 
  analogWrite(TFT_BACKLIGHT_PIN, 255); //max brightness
  DispInit(); //First SPI dev to init

  player.begin();
  while (!player.isChipConnected()) 
  {
     DispStatus("Couldn't find VS1053");
     player.begin();
     delay(100);
  }
  DispStatus("VS1053 Found");
  
  SDInit(); //todo:  verify success, SD is required
  SDLoadSettings();
  CurrentStationNum = DefaultStationNum;
  CurrentStation = RadioStation[DefaultStationNum];

  SelectButton.attach(SELECT_BUTTON_PIN ,  INPUT_PULLUP); // USE INTERNAL PULL-UP
  SelectButton.interval(5); 
  SelectButton.setPressedState(LOW); 
  CancelButton.attach(CANCEL_BUTTON_PIN ,  INPUT_PULLUP); // USE INTERNAL PULL-UP
  CancelButton.interval(5); 
  CancelButton.setPressedState(LOW); 

  Serial.println("ESP32 Internet Radio");
  Serial.println(VERSION);
  Serial.println("Setup Complete");  
  DispStatus("Setup Complete");  
  digitalWrite(BLUE_LED_PIN, false); // indicates streaming
  
#if SLAVE_MODE
  SerialSlave.begin(115200, SERIAL_8N1, SERIAL2_RXD, SERIAL2_TXD);
  Serial.println("\n*** Slave mode, will not respond to USB Serial commands!");
#else 
  if (WifiConnectDefault()) StreamConnectLoop(); //connect/stream automatically in normal mode
  Serial.println("\nNormal/USB command mode");
#endif

}

void loop()
{ //should be no menu displayed when looping at this top level
  MainLoopWork();

  SelectButton.update();
  if (SelectButton.pressed()) MenuMain();

  CancelButton.update();
  if (CancelButton.pressed()) StopStreaming();  //with no menu, cancel stops streaming
  
  int8_t RotEncCancMove = RotEncCancUpdate();
  if (RotEncCancMove) 
  {  //cancel encoder selects stations
     if (RotEncCancMove == 1)
     {
        if (CurrentStationNum == NumStations - 1) CurrentStationNum = 0; //circular
        else CurrentStationNum++;
     }
     else
     {
        if (CurrentStationNum == 0) CurrentStationNum = NumStations - 1; //circular
        else CurrentStationNum--;
     }
     CurrentStation = RadioStation[CurrentStationNum];
     StreamConnectLoop();
  }
  
  DispCheckUpdateTime();  //only in main loop, displayed in Menu area
}

void MainLoopWork()
{
  if (isStreaming)
  {
    if (player.data_request()) FeedDecoder();
    if (client.available()) ClientReadByte();
    if (ReConnect)
    {
       ReConnect = false;
       StreamConnectLoop();
    }
  }
  PrintUpdate();
  CheckForSerial();
}

void StreamConnectLoop()
{
  uint32_t StartMillis = millis();
  isStreaming = false;  
  digitalWrite(BLUE_LED_PIN, false);
  
  if (WiFi.status() != WL_CONNECTED)
  {
     DispStatus("WiFi Not Connected");
     return;
  }
   
  uint8_t NumTries = 3;
  
  char Stat[MAX_CHARS_PER_LINE + 4] = "try:";
  strcat(Stat, CurrentStation.friendlyName);
  Stat[MAX_CHARS_PER_LINE - 1]=0;  //Terminate at single line, if needed
  DispStatus(Stat);
  DispTrackName("");
  DispCurrentStation("");

  //player.softReset(); //Otherwise gets stuck if changing encoding type (ie ogg/mp3)
  player.switchToMp3Mode();  //avoids VS1053 board rework.   also does .softReset()
  player.loadDefaultVs1053Patches();
  player.setVolume(100);   //0-100, 100 is loudest
  
  Serial.println("Attempting to connect to:");
  Serial.printf("  Name: %s\n", CurrentStation.friendlyName);
  Serial.printf("  Host: %s\n", CurrentStation.host);
  Serial.printf("  Path: %s\n", CurrentStation.path);
  Serial.printf("  Port: %d\n", CurrentStation.port);

  while(NumTries--)
  {
    if (TryStreamConnect())
    {
      isStreaming = true;
      digitalWrite(BLUE_LED_PIN, true);
      DispCurrentStation(CurrentStation.friendlyName);
      DispStatus("Streaming...");
      Serial.printf("Took %d mS to conn to: %s\n", millis() - StartMillis, CurrentStation.friendlyName);
      return;
    }
    else
    {
      Serial.println("*** Connect Failed!\n");
      DispStatus(" Trying again...");
    }
  }
  Serial.println("*** Giving up!");
  DispStatus("Couldn't connect to stream");
}
  
bool TryStreamConnect()
{
  //client.stop();   
  Serial.printf("\nAttempting to connect to: %s\n", CurrentStation.friendlyName);  
 
  if (!client.connect(CurrentStation.host, CurrentStation.port)) 
  {
    Serial.println("Connection failed");
    return false;
  }
  
  Serial.printf("Requesting path: %s\n", CurrentStation.path);
  client.print(String("GET ") + CurrentStation.path + " HTTP/1.1\r\n" +
               "Host: " + CurrentStation.host + "\r\n" + 
               "Icy-MetaData:1\r\n" +
               "Connection: close\r\n\r\n");
  
  if (!ClientAvailableTimeout(3000)) return false;  //wait up to 3 seconds for data to start flowing
  
  MetaDataInterval = 0;
  if (!ReadMetaHeader()) return false;
  MetaDataPresent = (MetaDataInterval != 0);
  if (!MetaDataPresent) 
  {
    Serial.println("* No MetaData detected");
    DispTrackName("<unavailable>");
  }
  
  BytesUntilMetaData = MetaDataInterval;
  return ResetFillBuff();
}

bool WifiConnectDefault()
{
  return WifiConnect(WiFiSSID, WiFiPassword);
}

bool WifiConnect(const char *ssid, const char *password)
{
  if (ssid[0] == 0)
  {
     DispStatus("WiFi fail: No SSID");
     return false;
  }
  StopStreaming();
  DispStatus("Connecting to WiFi");
  Serial.printf("Connecting to SSID %s\n", ssid);
  WiFi.disconnect(); 
  delay(100);  //seems to resolve occasional 0dB RSSI when re-connecing to same (?)
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false); // Don't allow WiFi sleep
  WiFi.begin(ssid, password);
  
  uint8_t WaitCycles = 30; //~15 seconds (seeing 5 sec typical)
  while (WiFi.status() != WL_CONNECTED) 
  {
    if (WaitCycles-- == 0)
    {
      Serial.println("* Timeout waiting for Wifi Connect");
      DispStatus("WiFi fail: Timeout!");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("\nWiFi connected");  
  Serial.print("IP address: ");  Serial.println(WiFi.localIP());
  Serial.printf("Signal strength (RSSI): %d dBm\n", WiFi.RSSI());
  if (WiFi.RSSI() == 0) 
  {
    Serial.println("No Signal?");
    DispStatus("WiFi fail: no signal!");
    return false;
  }
  
  DispStatus("Starting UDP");
  Udp.begin(LOCAL_UDP_PORT);
  //Done at connect instead of each synch as this resolution can take >1 sec, causing audio out glitch (though typically <20mS)
  WiFi.hostByName(ntpServerName, ntpServerIP); // get a random server IP from the pool via name
  Serial.printf("Time server: %s     IP: ", ntpServerName);
  Serial.println(ntpServerIP);

  DispStatus("Synching time from 'net");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);  //seconds (300 in example)

  DispStatus("Connected to WiFi");
  return true;
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  Serial.printf("Updating time from: %s      IP: ", ntpServerName);
  Serial.println(ntpServerIP);
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  sendNTPpacket(ntpServerIP); //only 1-2mS

  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) 
  {
    if (isStreaming && player.data_request()) FeedDecoder(); //Keep playing from buffer while time being read
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) 
    {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.printf("Received NTP Response in %d mS\n", (millis() - beginWait));  //typical <120 mS or timeout
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("NTP Response timeout");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

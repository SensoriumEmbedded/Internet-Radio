
#define SERIAL_INPUT_TIMEOUT_MS  50 //mS to wait for each user input char when expecting command argument
#define MAX_URL_LEN             100 //max length of URL including Server, Port and Page
#define MAX_SSID_PW_LEN         100 //max length of Wifi SSID plus Password 

void CheckForSerial()
{
  if (!SerialSlave.available()) return;
   
  switch (SerialSlave.read())  
  {
    case 'c': //Connect to stream
      if (NewStreamInput()) StreamConnectLoop();
      else Serial.println("Could not interpret input");
      break;
    case 'd': //Debug/test
      //delay(500);
      //DispInit();
      break;
    case 'f': //Force Rebuffer
      ForceRebuf = !ForceRebuf;
      Serial.printf("Force Rebuf: %s\n", ForceRebuf ? "On" : "Off");
      break;
    case 'q': //quit streaming
      StopStreaming();
      break;
    case 's': //scan for wifi
      ScanWiFi();
      break;
    case 't': //toggle print updates
      PrintUpdates = !PrintUpdates;
      Serial.printf("Print Updates: %s\n", PrintUpdates ? "On" : "Off");
      break;
    case 'v': //version info
      SerialSlave.printf("ESP32 Internet Radio\r");
      Serial.println();
      SerialSlave.printf("%s\r", VERSION);
      Serial.println();
      break;
    case 'r': //set  PlayRate: doesn't seem to work, even with pitch patch...
      NewPlayRate();
      break;
    case 'w': //connect to wifi
      StopStreaming();
      SendPassFail(NewWifiInputConnect());
      break;
    case '?': //commands/help
      Serial.println("Serial interface commands:");
      Serial.println("   c<server><:port></path><\\r> Connect to stream");
      Serial.println("      example:  cquarrel.str3am.com:7110/live");
      Serial.println("   c: (re)Connect to currently selected stream");
      Serial.println("   f: Toggle Force Rebuffer on underrun");
      Serial.println("   q: Quit streaming");
      Serial.println("   s: Scan WiFi networks");
      Serial.println("   t: Toggle Periodic status updates");
      Serial.println("   v: Version Info");
      Serial.println("   r<+/-Rate><\\r>: Adjust playback rate");
      Serial.println("   w<SSID></><PW><\\r>: Connect to Wifi");
      Serial.println("   ?: This Command list");
      break;
    default:
      Serial.println("Unknown Command");		
  }
#if !SLAVE_MODE
  while(SerialAvailabeTimeout()) SerialSlave.read(); //flush the incoming buffer when using the Arduino Serial interface
#endif
}

bool SerialAvailabeTimeout()
{
  uint32_t StartMillis = millis();
  
  while(!SerialSlave.available() && millis() - StartMillis < SERIAL_INPUT_TIMEOUT_MS); // timeout loop
  
  return(SerialSlave.available());
}

bool GetSerialString(char * cpBuf, int iMaxLength)
{
  int iCharNum = 0;
  
  iMaxLength--; //always leave room for term char
  while (iCharNum < iMaxLength) 
  {
    if (!SerialAvailabeTimeout()) 
    {
      Serial.println("Serial Timeout!");  
      return(false);
    }
    cpBuf[iCharNum] = SerialSlave.read();
    if (cpBuf[iCharNum] == '\r') break; 
    iCharNum++;
  } 
  cpBuf[iCharNum] = 0; //terminate (or force if at max len)
  return(iCharNum < iMaxLength);  
}

bool NewStreamInput()
{
  char * StrFound = NULL;
  int NewPort = 0;
  
  // 'c' has been received, now read the URL info from user...
  char Buf[MAX_URL_LEN];
  if (!GetSerialString(Buf, MAX_URL_LEN)) return false; 
  if (strlen(Buf) == 0) return true; //no argument means re-start current stream
  if (strlen(Buf) < 5) return false;
  
  Serial.printf("Requested: %s\n", Buf);
  
  StrFound = strstr(Buf, ":");
  if (StrFound == NULL)
  {
    NewPort = 80; //default if not defined
    StrFound = strstr(Buf, "/"); //point to start of path
    if (StrFound == NULL) StrFound = Buf + strlen(Buf); //no path, point to end
  }
  else //port defined, read it
  {
     sscanf(StrFound + 1, "%d", &NewPort);
     if (NewPort == 0) return false;
  }      
  
  //*** we are now comitted to the change, update CurrentStation...
  
  memcpy(CurrentStation.host, Buf, StrFound - Buf);  //from start to beginning of path or port
  CurrentStation.host[StrFound - Buf]=0; //terminate it
  CurrentStation.port = NewPort;
  strcpy(CurrentStation.friendlyName, "User Set");

  StrFound = strstr(Buf, "/"); //point to start of path
  if (StrFound == NULL) *CurrentStation.path = 0; //no path, terminate at zero len
  else strcpy(CurrentStation.path, StrFound);
    
  return true;
}

void NewPlayRate()
{//Fine tune the data rate in PPM
   char Buf[20];
   if (!GetSerialString(Buf, 20)) 
   {
      Serial.println("Rate not set");
      return; 
   }
   long PlayRate;
   sscanf(Buf, "%d", &PlayRate);
   player.adjustRate(PlayRate);
   Serial.printf("Rate set to %d\n", PlayRate);
}

bool NewWifiInputConnect()
{
  char SSID_PW[MAX_SSID_PW_LEN];
  char * PW = NULL;
  
  if (!GetSerialString(SSID_PW, MAX_SSID_PW_LEN)) return false; 
  if (strlen(SSID_PW) < 4) return false;
  
  PW = strstr(SSID_PW, "/"); // SSID/PW delimiter
  if (PW == NULL) return false;
  *PW = 0; //terminate SSID
  PW++; //point to start of PW

  Serial.printf("Requested SSID: %s\n", SSID_PW);
  //Serial.printf("Requested PW: %s\n", PW);
  return WifiConnect(SSID_PW, PW);
}

void SendPassFail(bool PasFail)
{
  SerialSlave.print(PasFail ? "*Pass\r" : "*Fail\r");
  Serial.println();  //cr/lf combo for USB serial (ard monitor)
}

void ScanWiFi()
{
  StopStreaming();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("WiFi Scan start");
  int n = WiFi.scanNetworks(); // WiFi.scanNetworks will return the number of networks found
  Serial.println("Scan Complete");
#if SLAVE_MODE
  SerialSlave.printf("%d\r", n);
#endif
  if (n == 0) Serial.println("No networks found");
  else 
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) 
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)? "*Open" : " ");
#if SLAVE_MODE
      SerialSlave.print(WiFi.SSID(i));
      SerialSlave.printf("\r%d\r", WiFi.RSSI(i));
#endif
    }
  }
}


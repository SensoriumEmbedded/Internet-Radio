
void MenuMain()
{
   const StructMenuItem MainMenu[] = 
   {
     "Reconnect Station",      StreamConnectLoop,
     "Set Station as default", SetStationDefault,
     "Select New Station",     MenuStationSelect,  
     "Reconnect WiFi",         VoidWifiConnectDefault,       
     "Scan/Connect WiFi",      MenuWiFiScanConnect,
     "SD File select/disp",    PlayFile,  
     "Display 1941 Dial",      DispDial,
     "Display Brightness",     MenuDisplayBrightness,
     //"Save Settings",          SDSaveSettings,
     //"Load Settings",          SDLoadSettings,
     //"SD Card utils",          MenuSDUtils,
     //"Debug",                  DebugRoutine,
   };

   int16_t Result = DispMenuWait(MainMenu, sizeof(MainMenu)/sizeof(MainMenu[0]), "Main Menu"); 
   
   if (Result == MENU_CANCEL) 
   {
      DispMenuBlank(); //No Menu shown in main loop
      return;
   }
   
   MainMenu[Result].call_function(); //execute the function
   
   DispMenuBlank();  //No Menu shown in main loop
}

void SetStationDefault()
{
   DefaultStationNum = CurrentStationNum;
   SDSaveSettings();
}

void VoidWifiConnectDefault()
{  //stupid wrapper because WifiConnectDefault returns a bool  :(
   WifiConnectDefault();
}

void DebugRoutine()
{

}

void MenuStationSelect()
{
   //copy station names to a menu array
   StructMenuItem StationMenu[NumStations];
   for (uint16_t count = 0; count < NumStations; count++) strncpy(StationMenu[count].name, RadioStation[count].friendlyName, sizeof(StationMenu[count].name));
  
   int16_t Result = DispMenuWait(StationMenu, NumStations, "Select Station");
   
   if (Result == MENU_CANCEL) return;
   
   //Set newly selected station:
   CurrentStation = RadioStation[Result];
   CurrentStationNum = Result;
   StreamConnectLoop();
}

void MenuWiFiScanConnect()
{
  StopStreaming();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  DispStatus("WiFi Scan started");
  int NumNetworks = WiFi.scanNetworks(); // WiFi.scanNetworks will return the number of networks found
  //DispStatus("Scan Complete");
  if (NumNetworks == 0)
  {
     DispStatus("No networks found");
     return;
  }

  StructMenuItem cSSIDMenu[NumNetworks];
  char PrintBuf[32];
  
  sprintf(PrintBuf, "%d networks found", NumNetworks);
  DispStatus(PrintBuf);
  for (int i = 0; i < NumNetworks; ++i) 
  {
    // Print SSID and RSSI for each network found
    // Encryption: TKIP (WPA) = 2, WEP = 5, CCMP (WPA) = 4, NONE = 7, AUTO = 8
    Serial.printf("%d: %d rssi, enc %d, %s\n", i, WiFi.RSSI(i), WiFi.encryptionType(i), WiFi.SSID(i).c_str());
    
    strncpy(cSSIDMenu[i].name, WiFi.SSID(i).c_str(), sizeof(cSSIDMenu[i].name));
    cSSIDMenu[i].name[sizeof(cSSIDMenu[i].name) - 1] = 0; //terminate end in case not all coppied
  }

  int16_t SSIDResult = DispMenuWait(cSSIDMenu, NumNetworks, "Select Network");
  char Password[MAX_CHAR_INP_LEN] = {0};
  
  if (SSIDResult == MENU_CANCEL) return;
   
  //request password, if needed
  if (WiFi.encryptionType(SSIDResult) != WIFI_AUTH_OPEN)
  {
     if (DispCharMenuWait("PW: ", Password) == MENU_CANCEL) return;
  }
  
  //Connect to newly selected network:
  if (WifiConnect(cSSIDMenu[SSIDResult].name, Password))
  {
     strncpy(WiFiSSID, WiFi.SSID(SSIDResult).c_str(), MAX_SSID_CHAR_LEN);
     strncpy(WiFiPassword, Password, MAX_CHAR_INP_LEN);
     SDSaveSettings();
     DispStatus("WiFi connected, saved");
  } 

}

//void MenuSDUtils()
//{
//   StructMenuItem SDUtilMenu[] = 
//   {
//     "Play File", PlayFile,  
//     "Play MP3a",  SelectPlayMP3a,
//     "Play MP3b",  SelectPlayMP3b,
//   };
//
//   int16_t Result = DispMenuWait(SDUtilMenu, sizeof(SDUtilMenu)/sizeof(SDUtilMenu[0]), "SD Util Menu"); 
//   
//   if (Result == MENU_CANCEL) return;
//   
//   SDUtilMenu[Result].call_function(); //execute the function
//   
//}

void MenuDisplayBrightness()
{
   const StructMenuItem DispBrightMenu[] = 
   {
     " 10%",  NULL,
     " 20%",  NULL,
     " 30%",  NULL,
     " 40%",  NULL,
     " 50%",  NULL,
     " 60%",  NULL,
     " 70%",  NULL,
     " 80%",  NULL,
     " 90%",  NULL,
     "100%",  NULL,
   };

   int16_t Result = DispMenuWait(DispBrightMenu, sizeof(DispBrightMenu)/sizeof(DispBrightMenu[0]), "Disp Brightness Menu"); 
   
   if (Result == MENU_CANCEL) return;
   
   analogWrite(TFT_BACKLIGHT_PIN, (1+Result)*25.5);  //0-9  to 25-255
   //analogWrite(TFT_BACKLIGHT_PIN, (10-Result)*25.5);  //0-9  to 255-25
}

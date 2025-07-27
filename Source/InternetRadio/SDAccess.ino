
void SDInit()
{//todo: make this a test, return bool.  Once at setup() and menu selection?
    if(!SD.begin(SD_CARD_CS_PIN)){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    //uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", SD.cardSize()/1024/1024);
}

void PlayFile()
{
   char FullPath[300] = "/";
   int16_t Result = MENU_DIR_SEL; //start with root contents
   
   //SDInit();
   while (Result != MENU_FILE_SEL) //keep navigating until file selected (or cancel)
   { 
      Result = DispDirMenuWait(FullPath);
      if (Result == MENU_CANCEL) return;
      if (Result == MENU_UP_DIR)
      {
         char * LastSlash = strrchr(FullPath, '/'); //find last slash
         if (LastSlash != NULL) LastSlash[0] = 0;  //terminate it there
      }
      //if MENU_DIR_SEL, new dir is already appended to FullPath
      //Serial.printf("New path: %s\n", FullPath);
   } 

   Serial.printf("Selection: %s\n", FullPath);
   if (strlen(FullPath) < 6) return; //at least root slash plus 1 char plus extension
   
   char * Extension = FullPath + strlen(FullPath) - 4;  //last 4 chars
   for(size_t i = 0; i < 4; i++) Extension[i] = tolower(Extension[i]); //to lower case
   Serial.printf("Extension: %s\n", Extension);
   
   if (strcmp(Extension, ".mp3") == 0)
   {
      PlaySDMP3(FullPath);
   }
   else if (strcmp(Extension, ".jpg") == 0)
   {
      DispJPG(FullPath);
   }
   
}
   
void PlaySDMP3(const char * path)
{
   player.switchToMp3Mode();  //avoids VS1053 board rework.   also does .softReset()
   player.loadDefaultVs1053Patches();
   player.setVolume(100);   //0-100, 100 is loudest

   File file = SD.open(path);
	if (!file)
	{
      DispStatus("Unable to open file");
      return;
   }
   
   Serial.printf("Playing file: %s\n", path);
   DispCurrentStation("Playing from SD Card");
   DispTrackName(path);
   
   uint8_t audioBuffer[MP3_BUF_SIZE] __attribute__((aligned(4)));
   uint32_t StartMillis = millis();
   uint16_t LastElapsedSec = 9999;
   DispStatus(""); //just to clear it before direct writing below
   CancelButton.update();
   while (file.available() && !CancelButton.pressed())
   {
      int bytesRead = file.read(audioBuffer, MP3_BUF_SIZE);
      player.playChunk(audioBuffer, bytesRead);
      uint16_t ElapsedSec = (millis() - StartMillis)/1000;  //18 hours max w/ 16 bit
      if (LastElapsedSec != ElapsedSec)
      {
         tft.setTextColor(STATUS_FONT_COLOR, STATUS_BACK_COLOR); 
         tft.setCursor(0, DISP_STATUS_TOP + 3);
         tft.printf("Elapsed: %d:%02d", ElapsedSec/60, ElapsedSec % 60);            
         LastElapsedSec = ElapsedSec;
      }
      CancelButton.update();
   }
   file.close();
   Serial.printf("Finished in %d sec\n", (millis() - StartMillis)/1000);
   DispInit();
   if (isStreaming) ReConnect = true; //reconnect if we were streaming before
}

void SDSaveSettings()
{   
   DispStatus("Saving Settings");   
   File myFile = SD.open(SETTINGS_FILE_NAME, FILE_WRITE);
   if (!myFile) 
   {
     DispStatus("Error saving file!");
     return;
   }
   
   myFile.println(VERSION);
   myFile.printf("%d/%02d/%02d  %2d:%02d:%02d\r\n", year(), month(), day(), hour() , minute(), second());

   myFile.println(ntpServerName);
   myFile.println(timeZone);
   
   myFile.println(WiFiSSID);
   myFile.println(WiFiPassword);

   myFile.println(DefaultStationNum);
   for (int StatNum=0; StatNum < NumStations; StatNum++) 
   {
      myFile.println(RadioStation[StatNum].friendlyName);
      myFile.println(RadioStation[StatNum].host);
      myFile.println(RadioStation[StatNum].port);
      myFile.println(RadioStation[StatNum].path);
   }
   
   myFile.close();
   DispStatus("Save Complete"); 
}

void SDLoadSettings()
{  //wrapper to catch loading failures
   DispStatus("Loading Settings");   
   if (SDLoadSettingsFile()) DispStatus("Settings Loaded"); 
   else DispStatus("Load Settings Failed!"); 
}

bool SDLoadSettingsFile()
{   
   char buf[100];
   File myFile = SD.open(SETTINGS_FILE_NAME, FILE_READ);
   if (!myFile) return false;

   if (!SDReadLine(&myFile, buf, 10)) return false;  //FW VERSION when file saved (for ref)
   Serial.printf("Settings by FW v%s\n", buf);
   if (!SDReadLine(&myFile, buf, 100)) return false;  //Date/time saved
   Serial.printf("File saved %s\n", buf);

   if (!SDReadLine(&myFile, ntpServerName, 32)) return false;  //ntpServerName
      // NTP Servers:
      //static const char ntpServerName[] = "us.pool.ntp.org";
      //static const char ntpServerName[] = "time.nist.gov";
      //static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
      //static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
      //static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";
      //IPAddress timeServer(129,134,25,123);   
      //time.google.com    216,239,35,0
      //time.facebook.com  129,134,25,123
      //time.windows.com
      //time.apple.com
      //time-a-g.nist.gov   
   if (!SDReadLine(&myFile, buf, 10)) return false;  //time zone
   sscanf(buf, "%d", &timeZone);
      //timeZone = 1;     // Central European Time
      //timeZone = -5;  // Eastern Standard Time (USA)
      //timeZone = -4;  // Eastern Daylight Time (USA)
      //timeZone = -8;  // Pacific Standard Time (USA)
      //timeZone = -7;  // Pacific Daylight Time (USA)

   if (!SDReadLine(&myFile, WiFiSSID, MAX_SSID_CHAR_LEN)) return false;  
   if (!SDReadLine(&myFile, WiFiPassword, MAX_CHAR_INP_LEN)) return false; 

   if (!SDReadLine(&myFile, buf, 10)) return false;  //Default Station
   sscanf(buf, "%d", &DefaultStationNum);
   
   //read the station list:
   NumStations = 0;
   while (myFile.available() && NumStations < MAX_STATIONS) 
   {
      if (!SDReadLine(&myFile, RadioStation[NumStations].friendlyName, MAX_CHARS_PER_LINE)) return false;
      if (!SDReadLine(&myFile, RadioStation[NumStations].host, 64)) return false;      
      if (!SDReadLine(&myFile, buf, 10)) return false;  
      sscanf(buf, "%d", &RadioStation[NumStations].port);
      if (!SDReadLine(&myFile, RadioStation[NumStations].path, 64)) return false;
      NumStations++;
   }
   
   Serial.printf("%d stations read, settings complete\n", NumStations);
   myFile.close();
   return true;
}

bool SDReadLine(File * myFile, char * buf, int MaxLen)
{
  char NextChar;
  int CharCount = 0;
  
  do
  {
    if (CharCount == MaxLen || !myFile->available()) return false;
    NextChar = myFile->read();
    buf[CharCount++] = NextChar;
  } while(NextChar != '\r');
  if (myFile->read() != '\n') return false; //expecct \r\n (from println) on each line
  buf[CharCount-1] = 0;  //terminate it
  return(true);
}
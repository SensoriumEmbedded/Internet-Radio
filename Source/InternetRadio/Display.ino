
void DispInit()
{
  tft.init();
  tft.setRotation(DISP_ORIENTATIOM);
  MainLoopWork();
  tft.fillScreen(DISP_BACK_COLOR);
  MainLoopWork();
  
  tft.setSwapBytes(true); // swap the colour bytes (endianess) for jpg disp
  TJpgDec.setJpgScale(1); // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setCallback(DispOutputBMP);// JPG display callback

  tft.setCursor(0, 0);
  tft.setTextSize(4);
  tft.setTextColor(HEADLINE_FONT_COLOR); 

#ifdef PHILCO_1941_RADIO_HW
   tft.print(" 1941 internet Radio");
#else
   tft.print(" Trav's 'net Radio");
#endif
  
  tft.setTextSize(3);
  MainLoopWork();
  DispMenuBlank();  //blank on start
  if (isStreaming)
  {
     MainLoopWork();
     DispTrackName(TrackName);
     MainLoopWork();
     DispCurrentStation(CurrentStation.friendlyName);
     DispStatus("Streaming...");
  }
  else
  {
     DispTrackName("");
     DispCurrentStation("");
     DispStatus("Not Streaming.");
  }
}

void DispCheckUpdateTime()
{
  static uint16_t LastSec = 75;
  
  if (second() == LastSec) return;
  tft.setTextColor(TIME_FONT_COLOR, DISP_BACK_COLOR);
  tft.setCursor(10, DISP_MENU_TOP + DISP_PIX_PER_LINE);
  tft.printf("%d/%02d/%02d", year(), month(), day());
  tft.setCursor(240, DISP_MENU_TOP + DISP_PIX_PER_LINE);
  tft.printf("%2d:%02d:%02d", hour() , minute(), second());
  LastSec = second();
}

void DispMenuBlank()
{
   tft.fillRect(0, DISP_MENU_TOP, 480, DISP_MENU_HEIGHT, DISP_BACK_COLOR);  
}

void DispMenuItem(const StructMenuItem *MyMenu, const int NumItems, const int CurrentItem)
{
   tft.fillRect(1, DISP_MENU_TOP + 1 + DISP_PIX_PER_LINE, 479, 2 + 3 * DISP_PIX_PER_LINE, MENU_BACK_COLOR);  
   tft.setCursor(0, DISP_MENU_TOP + 3 + DISP_PIX_PER_LINE);
   
   int TopItem = 0; //default top of list at top
   if (CurrentItem > 0) TopItem = CurrentItem - 1;  //item in middle
   if (CurrentItem == NumItems - 1 && NumItems > 2) TopItem = NumItems - 3; //last item at bottom
   
   uint8_t ListLine = 0;
   while (ListLine < 3 && TopItem + ListLine < NumItems)
   {
      if (TopItem + ListLine == CurrentItem) tft.setTextColor(MENU_SELECT_FONT_COLOR, MENU_SELECT_BACK_COLOR); 
      else tft.setTextColor(MENU_FONT_COLOR, MENU_BACK_COLOR); 
      tft.println(MyMenu[TopItem + ListLine++].name); 
   }
}

int16_t DispMenuWait(const StructMenuItem *MyMenu, const int NumItems, const char *Title)
{
   int16_t CurrentSelection = 0;
   
   tft.fillRect(0, DISP_MENU_TOP, 480, DISP_MENU_HEIGHT, MENU_BACK_COLOR);
   tft.setCursor(0, DISP_MENU_TOP + 3);
   tft.setTextColor(MENU_TITLE_COLOR); 
   tft.print(Title);
   DispMenuItem(MyMenu, NumItems, CurrentSelection);

   while(1)
   {
      MainLoopWork();

      SelectButton.update();
      if (SelectButton.pressed()) return CurrentSelection;

      CancelButton.update();
      if (CancelButton.pressed())
      {
         DispStatus("Canceled");
         return MENU_CANCEL;
      }
      int8_t RotEncSelMove = RotEncSelUpdate();
      if (RotEncSelMove) 
      {
         if (RotEncSelMove == 1)
         {
            if (CurrentSelection != NumItems - 1) DispMenuItem(MyMenu, NumItems, ++CurrentSelection);
            //else CurrentSelection = 0; //circular
         }
         else
         {
            if (CurrentSelection != 0) DispMenuItem(MyMenu, NumItems, --CurrentSelection);
            //else CurrentSelection = NumItems - 1; //circular
         }
         //DispMenuItem(MyMenu, NumItems, CurrentSelection);
      }
   }

}

void DispCharMenuItem(const char * CharList, int ItemNum, bool Selected)
{  //highlight or unhighlight selected char/field
   int NumChars = strlen(CharList);
   int Count = 0;

   tft.setCursor(0, DISP_MENU_TOP + 3);
   tft.setTextColor(MENU_FONT_COLOR); //transparent background for spacing
   if (ItemNum < NumChars) //just a char to update
   {
      while(Count++ < ItemNum) tft.print(" ");
      
      if (Selected) tft.setTextColor(MENU_SELECT_FONT_COLOR, MENU_SELECT_BACK_COLOR); 
      else tft.setTextColor(MENU_FONT_COLOR, MENU_BACK_COLOR); 
      tft.print(CharList[ItemNum]);
   }
   else //field update
   {
      //tft.print(" Del Done"); //this is what is after char list
      while(Count++ < NumChars + 1) tft.print(" "); //skip past chars, plus one space
      if (ItemNum > NumChars) tft.print("    ");  //skip "Del "

      if (Selected) tft.setTextColor(MENU_SELECT_FONT_COLOR, MENU_SELECT_BACK_COLOR); 
      else tft.setTextColor(MENU_FONT_COLOR, MENU_BACK_COLOR); 
      if (ItemNum == NumChars) tft.print("Del");  
      else tft.print("Done");  
   }
}

int16_t DispCharMenuWait(const char *Title, char * Result)
{
   char CharList[100];
   int NumChars = 0;
   
   //The allowable characters are: ASCII 0x20, 0x21, 0x23, 0x25 through 0x2A, 
   //                0x2C through 0x3E, 0x40 through 0x5A, 0x5E through 0x7E.
   for(char ThisChar = 0x40; ThisChar <= 0x5A; ThisChar++) CharList[NumChars++] = ThisChar;
   for(char ThisChar = 0x5E; ThisChar <= 0x7E; ThisChar++) CharList[NumChars++] = ThisChar;
   CharList[NumChars++] = 0x20;
   CharList[NumChars++] = 0x21;
   CharList[NumChars++] = 0x23;
   for(char ThisChar = 0x25; ThisChar <= 0x2A; ThisChar++) CharList[NumChars++] = ThisChar;
   for(char ThisChar = 0x2C; ThisChar <= 0x3E; ThisChar++) CharList[NumChars++] = ThisChar;

   CharList[NumChars] = 0;  //terminate the list
   Serial.printf("%d allowable chars\n%s\n", NumChars, CharList); //88 allowable chars
   
   //DispMenuItem(CharList);
   //tft.print(Title);
   DispStatus(Title);  //todo: need better place to display this char menu title
   
   //display full char menu
   tft.fillRect(0, DISP_MENU_TOP, 480, DISP_MENU_HEIGHT, MENU_BACK_COLOR);
   tft.setCursor(0, DISP_MENU_TOP + 3);
   tft.setTextColor(MENU_FONT_COLOR); 
   tft.print(CharList);
   tft.print(" Del Done");
   
   int16_t CurrentSelection = 0;
   int8_t ResLength = 0;
   
   DispCharMenuItem(CharList, CurrentSelection, true);  //highlight First/selected
   Result[0] = 0;  //Terminate result at 0 len
   
   while(1)
   {
      MainLoopWork();

      SelectButton.update();
      if (SelectButton.pressed())
      {
         if (CurrentSelection < NumChars) //char selected, add to Result
         {  
            if (ResLength < MAX_CHAR_INP_LEN - 1)
            {
               Result[ResLength] = CharList[CurrentSelection];
               Result[++ResLength] = 0;  //inc, Terminate
            }
         }
         else //field selected, do action
         {
            if (CurrentSelection == NumChars) //tft.print("Del");  
            {
               if (ResLength > 0) Result[--ResLength] = 0;  //dec, Terminate                  
            }
            else return MENU_OK; //tft.print("Done");  
         }
         DispStatus(Result);
      }  //Select Button Pressed
      
      CancelButton.update();
      if (CancelButton.pressed()) 
      {
         DispStatus("Canceled");
         return MENU_CANCEL;
      }
      
      int8_t RotEncSelMove = RotEncSelUpdate();
      if (RotEncSelMove) 
      {
         DispCharMenuItem(CharList, CurrentSelection, false);  //unhighlight previous
         if (RotEncSelMove == 1)
         {
            if (CurrentSelection == NumChars + 2 - 1) CurrentSelection = 0; //circular
            else CurrentSelection++;
         }
         else
         {
            if (CurrentSelection == 0) CurrentSelection = NumChars + 2 - 1; //circular
            else CurrentSelection--;
         }
         DispCharMenuItem(CharList, CurrentSelection, true);  //highlight new selection
      }
   }

}

int16_t DispDirMenuWait(char *DirName)
{
   const uint16_t MaxDirItems = 100;
   StructMenuItem DirMenu[MaxDirItems];
   uint16_t ItemNum = 0;
   
   //SDInit();
   
   Serial.printf("Listing directory: %s\n", DirName);

   File root = SD.open(DirName);
   if(!root){
       DispStatus("Failed to open dir");
       return MENU_CANCEL;
   }
   if(!root.isDirectory()){
       DispStatus("Not a directory");
       return MENU_CANCEL;
   }
   
   File entry = root.openNextFile();
   if (strcmp(DirName, "/") != 0) strcpy(DirMenu[ItemNum++].name, UP_DIR_STRING);

   while(entry && ItemNum < MaxDirItems){
       if(entry.isDirectory())
       {
           Serial.printf(" Dir: %s\n", entry.name());
           strcpy(DirMenu[ItemNum].name, "/");
           strncat(DirMenu[ItemNum].name, entry.name(), sizeof(DirMenu[0].name)-1); //leave room for '/'
       } 
       else 
       {
           Serial.printf("File: %s\t\tSize: %d\n", entry.name(), entry.size());
           strncpy(DirMenu[ItemNum].name, entry.name(), sizeof(DirMenu[0].name));
       }
       entry = root.openNextFile();
       ItemNum++;
   }
   //todo: Sort dir list?  Filter by selectable type?
   //todo: allowance for over MaxDirItems?
   if (ItemNum == 0)
   { //todo: special return to do nothing?  shouldn't happen...
      DispStatus("Dir is empty");
      return MENU_CANCEL;
   }

   int16_t Result = DispMenuWait(DirMenu, ItemNum, "Dir Menu"); 
   if (Result == MENU_CANCEL) return MENU_CANCEL;
   
   if (strcmp(DirMenu[Result].name, UP_DIR_STRING) == 0) return MENU_UP_DIR;   
   
   int16_t SelectionType = MENU_DIR_SEL;
   
   if (DirMenu[Result].name[0] != '/')  
   {//it's a file
      strcat(DirName, "/"); //add a slash before file name
      SelectionType = MENU_FILE_SEL;
   }
   
   strcat(DirName, DirMenu[Result].name);
   return SelectionType;
}

int8_t RotEncSelUpdate()
{
   int newEncPos;
   static int EncPos = RotEncSel.getPosition();

   RotEncSel.tick();
   newEncPos = RotEncSel.getPosition();
   //Serial.print("pos:");
   //Serial.print(newEncPos);
   //Serial.print(" dir:");
   //Serial.println((int)(RotEncSel.getDirection()));
   if (EncPos != newEncPos) 
   {
      EncPos = newEncPos;
      return (int)RotEncSel.getDirection();
   }
   return 0;
}

int8_t RotEncCancUpdate()
{
   int newEncPos;
   static int EncPos = RotEncCanc.getPosition();

   RotEncCanc.tick();
   newEncPos = RotEncCanc.getPosition();
   //Serial.print("pos:");
   //Serial.print(newEncPos);
   //Serial.print(" dir:");
   //Serial.println((int)(RotEncCanc.getDirection()));
   if (EncPos != newEncPos) 
   {
      EncPos = newEncPos;
      return (int)RotEncCanc.getDirection();
   }
   return 0;
}

void DispStatus(const char *Status)
{
    tft.setTextColor(STATUS_FONT_COLOR); 
    tft.fillRect(0, DISP_STATUS_TOP, 480, DISP_STATUS_HEIGHT, STATUS_BACK_COLOR);
    tft.setCursor(0, DISP_STATUS_TOP + 3);
    tft.print(Status);
}

void DispCurrentStation(const char *StationName)
{
    tft.setTextColor(STATION_FONT_COLOR); 
    tft.fillRect(0, DISP_STATION_TOP, 480, DISP_STATION_HEIGHT, STATION_BACK_COLOR);
    tft.setCursor(0, DISP_STATION_TOP + 3);
    tft.print(StationName);
}

void DispTrackName(const char *TrackName)
{
    tft.setTextColor(TRACKNAME_FONT_COLOR); 
    tft.fillRect(0, DISP_TRACK_NAME_TOP, 480, DISP_TRACK_NAME_HEIGHT, TRACKNAME_BACK_COLOR); 
    tft.setCursor(0, DISP_TRACK_NAME_TOP + 3);
    tft.print(TrackName);
}

void DispDial()
{
   DispJPG("//DialOnly.jpg");
}

void DispJPG(const char *filename) 
{
   uint32_t t = millis();
   uint16_t w = 0, h = 0;
   
   //SDInit();
   tft.fillScreen(TFT_BLACK);
   TJpgDec.getSdJpgSize(&w, &h, filename);
   Serial.printf("File '%s'   Width = %d, Height = %d\n", filename, w, h);
   TJpgDec.drawSdJpg(0, 0, filename); // Draw the image, top left at 0,0
   
   t = millis() - t;
   Serial.print(t); Serial.println(" ms");
   
   do
   { 
     MainLoopWork();
     CancelButton.update();
   } while(!CancelButton.pressed());

   DispInit();
}

// This function will be called during decoding of the jpeg file to render each block to the TFT.  
bool DispOutputBMP(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   if ( y >= tft.height() ) return 0; // Stop further decoding as image is running off bottom of screen
   MainLoopWork();
   tft.pushImage(x, y, w, h, bitmap); // This function will clip the image block rendering automatically at the TFT boundaries
   return 1; // Return 1 to decode next block
}

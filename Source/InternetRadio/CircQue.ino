
#define CIRC_BUF_SIZE 65536 //Use most of available RAM, compiler indicates more available than there really is...
#define START_BUF_PCT 0.10
#define MP3_BUF_SIZE 32  // "vs1053 likes 32 bytes at a time" but "The buffer size 64 seems to be optimal. At 32 and 128 the sound might be brassy."
                         // Some stations (ie Fusion 101) sound "under water" with 64
                         
uint32_t BufHead, BufTail;
uint8_t CircBuf[CIRC_BUF_SIZE] __attribute__((aligned(4)));
uint8_t mp3buff[MP3_BUF_SIZE] __attribute__((aligned(4)));

bool DataObtained = true; //must start true
uint32_t UpDownTimeMillis;


uint32_t BufUsed()
{
   return (BufHead >= BufTail) ? (BufHead - BufTail) : (BufHead + CIRC_BUF_SIZE - BufTail );
}

bool ResetFillBuff()
{
  //reset the queue, prime the buffer
  BufHead = 0;
  BufTail = 0;
  DataObtained = true; 
  
  Serial.printf("Filling buffer to %d%%...\n", int(100.0*START_BUF_PCT));
  while (BufHead < CIRC_BUF_SIZE * START_BUF_PCT) 
  {
      if (!ClientAvailableTimeout(1000)) //Wait up to a 1000mS per byte (should be much faster)
      {
         Serial.println("\nTimeout waiting for data!");  
         return false;
      }
      ClientReadByte();
      if (BufHead != BufTail && BufHead % (CIRC_BUF_SIZE/10) == 0) Serial.printf("%d%%\n", (100*BufHead/CIRC_BUF_SIZE));
  }

  UpDownTimeMillis = millis();
  Serial.println("done, playing stream...");  
  return true;
}

void FeedDecoder()
{
  int toWrite = (BufUsed() > MP3_BUF_SIZE) ? MP3_BUF_SIZE : BufUsed();
  
  if (toWrite)
  {
     if (BufTail + toWrite < CIRC_BUF_SIZE)
     {//easy copy
        memcpy(mp3buff, CircBuf + BufTail, toWrite);
        BufTail += toWrite;
     }
     else
     {//tail wrap around
        uint32_t BytesAtTop = CIRC_BUF_SIZE - BufTail;
        memcpy(mp3buff, CircBuf + BufTail, BytesAtTop);
        BufTail = toWrite - BytesAtTop;
        memcpy(mp3buff + BytesAtTop, CircBuf, BufTail);
     }
     player.playChunk(mp3buff, toWrite);
     
     //detect and time under-runs
     if (!DataObtained) //we got data when we were down
     {
        Serial.printf(" Recovered after %d ms down\n", millis() - UpDownTimeMillis);
        UpDownTimeMillis = millis();
        DataObtained = true;
     }
  }
  else  //no data available to write
  {
     if (DataObtained) //no data when we were up
     {
        Serial.printf("No data after %d mS up\n", millis() - UpDownTimeMillis);
        UpDownTimeMillis = millis();
        DataObtained = false;
     }
     
     if (ForceRebuf)
     {
        Serial.print("Underrun...");
        if (!ResetFillBuff()) StopStreaming();
     }
     else
     {
        if (millis() - UpDownTimeMillis > 3000) 
        {
           Serial.println("3 sec data timeout!");           
           StopStreaming();
        }           
     }
  }
  //Serial.print(".");  //good for debug, but outputs a lot of dots...
}

void AddToQueue(uint8_t c)
{
  BytesUntilMetaData--;
  //Serial.printf("%05d\t%03d\t'%c'\n", BufHead, c, (c >= 32) ? c : '-'); //debug data dump
  
  CircBuf[BufHead++] = c; 
  if (BufHead == CIRC_BUF_SIZE) 
  {
     BufHead = 0;
     //Serial.printf("Head wrap\n");
  }
}

void PrintUpdate()
{
  if (!PrintUpdates) return;
  if (millis() - LastUpdateMs < 1000) return;

  Serial.printf("%08d\t%d\t%d%%\t%s\tMetaD:%s\tForceR:%s\tMinFreeHeap:%d\n", millis(), BufUsed(), 100*BufUsed()/CIRC_BUF_SIZE, isStreaming ? "Streaming" : "Stopped", MetaDataPresent ? "On" : "Off", ForceRebuf ? "On" : "Off", esp_get_minimum_free_heap_size()); //esp_get_minimum_free_heap_size   ESP.getFreeHeap()
  LastUpdateMs = millis();
}

bool ClientAvailableTimeout(uint32_t TimeoutMs)
{    
  uint32_t StartMillis = millis();
  
  while(!client.available() && millis() - StartMillis < TimeoutMs); // timeout loop
  
  return(client.available());
}

void StopStreaming()
{
   isStreaming = false;
   digitalWrite(BLUE_LED_PIN, false);
   DispStatus("Streaming stopped");
   DispTrackName("");
   DispCurrentStation("");
   Serial.println("Streaming stopped");
}
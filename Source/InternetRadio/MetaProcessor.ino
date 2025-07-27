
bool ReadMetaHeader()
{
  while (client.available()) 
  {
    String responseLine = client.readStringUntil('\n');
    
    // If we have an EMPTY header (or just a CR) that means we had two linefeeds
    if (responseLine[0] == (uint8_t)13 || responseLine == "") return true;
    
    Serial.printf("HEADER: ");
    Serial.println(responseLine);  

    if (responseLine.startsWith("icy-metaint")) // bytes between meta data blocks
    {
    	MetaDataInterval = responseLine.substring(12).toInt();
    	Serial.printf("Metadata Interval:%d\n", MetaDataInterval);
    }
    
  }
  Serial.println("* no double LF detected");
  return false;
}

void ReadMetaData()
{
  uint16_t CharNum = 0;
  uint16_t Len = (uint16_t)client.read() * 16; //up to 4080 bytes
  
  if (Len != 0) //most blocks are 0
  {
     Serial.printf("*** %05d\tRaw Meta (%d): ", millis()/1000, Len);

     while (Len--)
     {
        //while (!client.available()) Serial.println("***************"); //debug, never seems to happen
        if (isStreaming && player.data_request()) FeedDecoder(); //Keep playing from buffer while meta being received
        
        uint8_t c = client.read();
        if ( c >= 0x7F || (c < 0x20 && c != 0 && c != 13)) 
        {
          Serial.printf("Bad char(%d)\nTurning Metadata off and reconnecting\n", c);
          //PrintUpdates = false; //debug, stop serial updates
          MetaDataPresent = false; 
          DispTrackName("<bad data>");
          ReConnect = true; //re-start stream to re-synch meta and mp3 frames, too deep to reconnect from here
          return;
        }
        
        if (CharNum != MAX_TITLE_LEN - 1) TrackName[CharNum++] = c;
        else TrackName[CharNum] = 0;
     }
     
     Serial.println(TrackName);  //raw, pre-trim
     
     TrimMeta(TrackName);
     Serial.printf("*** %05d\tTrimmed Meta: %s\n", millis()/1000, TrackName);
     //Serial.println(TrackName);
     DispTrackName(TrackName);
  }
  BytesUntilMetaData = MetaDataInterval;
}

void TrimFromEnd(char * TrimFrom, char cToTrim)
{
  uint16_t StrLen = strlen(TrimFrom);
  if (StrLen > 0) 
  {
    StrLen--;
    if (TrimFrom[StrLen] == cToTrim) TrimFrom[StrLen] = 0;
  }
}

void TrimMeta(char * TrackBuf) 
{  //trim/format extra info/chars
  char *cptr;
  
  //remove StreamUrl to end, if present
  cptr = strstr(TrackBuf, "StreamUrl=");
  if (cptr != NULL) *cptr = 0; //terminate

  TrimFromEnd(TrackBuf, ';');
  TrimFromEnd(TrackBuf, '\'');

  //replace " - " with NL  (used on many channels)
  while ((cptr = strstr(TrackBuf, " - ")) != NULL)
  {  
    *cptr++ = '\n'; //insert a NL
    while (cptr[2]) 
    {  //move everything up
       *cptr = cptr[2];
       cptr++;
    }
    *cptr = 0; // terminate it
  }

  //replace "^" with NL   (Used on 'Jazz 88', others?)
  while ((cptr = strstr(TrackBuf, "^")) != NULL) *cptr = '\n'; 

  //remove "StreamTitle='"  (used on all metadata)
  cptr = strstr(TrackBuf, "StreamTitle='");
  if (cptr != NULL)
  {  
    while (cptr[13]) 
    {  //move everything up
       *cptr = cptr[13];
       cptr++;
    }
    *cptr = 0; // terminate it
  }

   if (strlen(TrackBuf) > MAX_TRACK_CHARS) TrackBuf[MAX_TRACK_CHARS] = 0;
}
     
void ClientReadByte()
{
  if (MetaDataPresent && BytesUntilMetaData == 0)
  {//always read metadata if it is time (doesn't add to queue)
    ReadMetaData();
    return; 
  }
  
  if (BufUsed() < CIRC_BUF_SIZE - MP3_BUF_SIZE) AddToQueue(client.read()); //don't read audio data if queue full
}


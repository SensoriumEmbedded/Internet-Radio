
#define SLAVE_MODE     false   //if true, All input and some output routes from/to SerialSlave instead of USB serial 
//#define PHILCO_1941_RADIO_HW            //if defined, use settings for 1941 Philco Radio build

#define VERSION "0.80"
// v0.80  2021/06/20: basic remote functions implemented, versioninng start

#define LOCAL_UDP_PORT       8888 // local port to listen for UDP packets

//      TFT_MISO           19   // Set in libraries\TFT_eSPI\User_Setup.h
//      TFT_MOSI           23   // Set in libraries\TFT_eSPI\User_Setup.h
//      TFT_SCK            18   // Set in libraries\TFT_eSPI\User_Setup.h
//      TFT_CS             15   // Set in libraries\TFT_eSPI\User_Setup.h
//      TFT_DC_RS           0   // Set in libraries\TFT_eSPI\User_Setup.h
#define TFT_BACKLIGHT_PIN   4   // Backlight PWM pin
#define SD_CARD_CS_PIN      5
#define VS_TFT_RESET_PIN   13   //Reset of VS1053 and TFT Display
#define VS1053_CS_PIN      32   // VS1053 chip select pin (output)
#define VS1053_DCS_PIN     33   // VS1053 Data/command select pin (output)
#define VS1053_DREQ_PIN    25   // VS1053 Data request, ideally an Interrupt pin
#define CANCEL_BUTTON_PIN  12
#define SELECT_BUTTON_PIN  14
#define ROT_ENC_SEL_0_PIN  27
#define ROT_ENC_SEL_1_PIN  26
#define ROT_ENC_CANC_0_PIN 21
#define ROT_ENC_CANC_1_PIN 22
#define BLUE_LED_PIN        2   //On-board Blue LED

#if SLAVE_MODE
   #define SERIAL2_RXD    16      //pindef for Serial2
   #define SERIAL2_TXD    17      //pindef for Serial2
   #define SerialSlave    Serial2 //control interface 
#else
   #define SerialSlave    Serial  //Standard USB 
#endif

//***** Display Related *****

//display is 480x320
#define DISP_PIX_PER_LINE      26
#define DISP_MENU_HEIGHT       (4 * DISP_PIX_PER_LINE + 2)    //  54
#define DISP_STATUS_HEIGHT     (1 * DISP_PIX_PER_LINE + 2)    //  28
#define DISP_STATION_HEIGHT    (1 * DISP_PIX_PER_LINE + 2)    //  28
#define DISP_TRACK_NAME_HEIGHT (4 * DISP_PIX_PER_LINE + 2)    // 132 

#define DISP_MENU_TOP          (320 - DISP_MENU_HEIGHT) // at bottom
#define DISP_STATUS_TOP        (DISP_MENU_TOP - DISP_STATUS_HEIGHT - 7) // over Station name, with gap
#define DISP_TRACK_NAME_TOP    (DISP_STATUS_TOP - DISP_TRACK_NAME_HEIGHT - 7) // over status, with gap
#define DISP_STATION_TOP       (DISP_TRACK_NAME_TOP - DISP_STATION_HEIGHT) // over track name

#define DISP_BACK_COLOR        TFT_BLACK   //TFT_PURPLE
#define STATUS_FONT_COLOR      TFT_YELLOW
#define STATUS_BACK_COLOR      TFT_BLUE
#define TIME_FONT_COLOR        TFT_LIGHTGREY
#define MENU_FONT_COLOR        TFT_BLACK
#define MENU_TITLE_COLOR       TFT_WHITE
#define MENU_BACK_COLOR        TFT_DARKGREY
#define MENU_SELECT_FONT_COLOR TFT_BLACK
#define MENU_SELECT_BACK_COLOR TFT_SKYBLUE
#define STATION_FONT_COLOR     TFT_YELLOW
#define STATION_BACK_COLOR     TFT_OLIVE
#define HEADLINE_FONT_COLOR    TFT_ORANGE
#define TRACKNAME_FONT_COLOR   TFT_GOLD
#define TRACKNAME_BACK_COLOR   TFT_DARKGREEN

#define MENU_CANCEL            -1  //Menu return code if not index to menu item
#define MENU_OK                -2  //Menu return code if valid, but not index to menu item
#define MENU_DIR_SEL           -3  //directory selected in file select menu
#define MENU_FILE_SEL          -4  //file selected in file select menu
#define MENU_UP_DIR            -5  //Up level directory selcted

#define MAX_CHAR_INP_LEN       21  //used for WiFi PW and other char string inputs, including term char
#define MAX_SSID_CHAR_LEN      34  //Max SSID length stored, incl term. Spec max is 32 chars
#define MAX_CHARS_PER_LINE     27  //26 chars in display line (plus term) 

#ifdef PHILCO_1941_RADIO_HW
   #define DISP_ORIENTATIOM        1 //1=SD card down
#else
   #define DISP_ORIENTATIOM        3 //3=SD card up (easier for stand-alone use)
#endif

#define UP_DIR_STRING          "/..<up dir>"

#define SETTINGS_FILE_NAME     "/NetRadio.txt"
#define MAX_TITLE_LEN          500 //max length for full/raw Song Title Meta Data (beyond will be terminated)
#define MAX_TRACK_CHARS        104 //Trim final to, 4 lines * 26 chars in display line

#define MAX_STATIONS            50 //max number of stations stored

struct StructMenuItem
{
  char name[MAX_CHARS_PER_LINE];
  void (*call_function)();
};

struct StructStationInfo
{
    char friendlyName[MAX_CHARS_PER_LINE]; 
    char host[64];
    uint16_t port;
    char path[64];
};

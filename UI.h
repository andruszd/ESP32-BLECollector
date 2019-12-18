
/*

  ESP32 BLE Collector - A BLE scanner with sqlite data persistence on the SD Card
  Source: https://github.com/tobozo/ESP32-BLECollector

  MIT License

  Copyright (c) 2018 tobozo

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  -----------------------------------------------------------------------------

*/


enum TextDirections {
  ALIGN_FREE   = 0,
  ALIGN_LEFT   = 1,
  ALIGN_RIGHT  = 2,
  ALIGN_CENTER = 3,
};


enum BLECardThemes {
  IN_CACHE_ANON = 0,
  IN_CACHE_NOT_ANON = 1,
  NOT_IN_CACHE_ANON = 2,
  NOT_IN_CACHE_NOT_ANON = 3
};

typedef enum {
  TFT_SQUARE = 0,
  TFT_PORTRAIT = 1,
  TFT_LANDSCAPE = 2
} DisplayMode;


static uint8_t percentBoxSize;
static uint8_t headerLineHeight;
static uint8_t leftMargin;
static uint8_t iconR;// = 4; // BLE icon radius
static uint8_t macAddrColorsScaleX;
static uint8_t macAddrColorsScaleY;

static int16_t graphX;
static int16_t graphY;
static int16_t percentBoxX;
static int16_t percentBoxY;
static int16_t headerStatsX;
static int16_t footerBottomPosY;
static int16_t headerStatsIconsX;
static int16_t headerStatsIconsY;
static int16_t progressBarY;
static int16_t hhmmPosX;
static int16_t hhmmPosY;
static int16_t uptimePosX;
static int16_t uptimePosY;
static int16_t copyleftPosX;
static int16_t copyleftPosY;
static int16_t cdevcPosX;
static int16_t cdevcPosY;
static int16_t sesscPosX;
static int16_t sesscPosY;
static int16_t ndevcPosX;
static int16_t ndevcPosY;
static int16_t gpsIconPosX;
static int16_t gpsIconPosY;
// icon positions for RTC/DB/BLE
static int16_t iconAppX;
static int16_t iconAppY;
static int16_t iconRtcX;
static int16_t iconRtcY;
static int16_t iconBleX;
static int16_t iconBleY;
static int16_t iconDbX;
static int16_t iconDbY;
static int16_t macAddrColorsPosX;
static int16_t filterVendorsIconX;
static int16_t filterVendorsIconY;
static int16_t heapStrX;
static int16_t heapStrY;
static int16_t entriesStrX;
static int16_t entriesStrY;

static uint16_t macAddrColorsSizeX;
static uint16_t macAddrColorsSizeY;
static uint16_t headerHeight;
static uint16_t footerHeight;
static uint16_t scrollHeight;
static uint16_t graphLineWidth;
static uint16_t graphLineHeight;

static int32_t macAddrColorsSize;

static bool showScanStats;
static bool showHeap;
static bool showEntries;
static bool showCdevc;
static bool showSessc;
static bool showNdevc;

static TextDirections entriesAlign;
static TextDirections heapAlign;
static TextDirections cdevcAlign;
static TextDirections sesscAlign;
static TextDirections ndevcAlign;

static char lastDevicesCountSpacer[5];// = ""; // Last
static char seenDevicesCountSpacer[5];// = ""; // Seen
static char scansCountSpacer[5];// = ""; // Scans


// UI palette
static const uint16_t BLE_WHITE       = 0xFFFF;
static const uint16_t BLE_BLACK       = 0x0000;
static const uint16_t BLE_GREEN       = 0x07E0;
static const uint16_t BLE_YELLOW      = tft_color565(0xff, 0xff, 0x00); // 0xFFE0;
static const uint16_t BLE_GREENYELLOW = 0xAFE5;
static const uint16_t BLE_CYAN        = 0x07FF;
static const uint16_t BLE_ORANGE      = 0xFD20;
static const uint16_t BLE_DARKGREY    = 0x7BEF;
static const uint16_t BLE_LIGHTGREY   = 0xC618;
static const uint16_t BLE_RED         = 0xF800;
static const uint16_t BLE_DARKGREEN   = 0x03E0;
static const uint16_t BLE_DARKBLUE    = tft_color565(0x22, 0x22, 0x44);
static const uint16_t BLE_PURPLE      = 0x780F;
static const uint16_t BLE_PINK        = 0xF81F;
static const uint16_t BLE_TRANSPARENT = TFT_TRANSPARENT;

// top and bottom non-scrolly zones
static const uint16_t HEADER_BGCOLOR      = tft_color565(0x22, 0x22, 0x22);
static const uint16_t FOOTER_BGCOLOR      = tft_color565(0x22, 0x22, 0x22);
// BLECard info styling
static const uint16_t IN_CACHE_COLOR      = tft_color565(0x37, 0x6b, 0x37);
static const uint16_t NOT_IN_CACHE_COLOR  = tft_color565(0xa4, 0xa0, 0x5f);
static const uint16_t ANONYMOUS_COLOR     = tft_color565(0x88, 0xaa, 0xaa);
static const uint16_t NOT_ANONYMOUS_COLOR = tft_color565(0xee, 0xee, 0xee);
// one carefully chosen blue
static const uint16_t BLUETOOTH_COLOR     = tft_color565(0x14, 0x54, 0xf0);
static const uint16_t BLE_DARKORANGE      = tft_color565(0x80, 0x40, 0x00);
// middle scrolly zone
static const uint16_t BLECARD_BGCOLOR     = tft_color565(0x22, 0x22, 0x44);
// placehorder for **variable** background color
static uint16_t BGCOLOR                   = tft_color565(0x22, 0x22, 0x44);


// heap map settings
#define HEAPMAP_BUFFLEN 61 // graph width (+ 1 for hscroll)


// heap management (used by graph)
static uint32_t min_free_heap = 90000; // sql out of memory errors eventually occur under 100000
static uint32_t initial_free_heap = freeheap;
static uint32_t heap_tolerance = 20000; // how much memory under min_free_heap the sketch can go and recover without restarting itself
static uint32_t heapmap[HEAPMAP_BUFFLEN] = {0}; // stores the history of heapmap values
static uint16_t heapindex = 0; // index in the circular buffer

size_t devicesStatCount = 0;    // how many devices found since last measure
unsigned long lastDeviceStatCount = 0; // when the last devices count reset was made

unsigned long devGraphFirstStatTime = millis();
unsigned long devGraphStartedSince = 0;
const unsigned long devGraphPeriodShort = 1000; // refresh every 1 second
const unsigned long devGraphPeriodLong  = 1000 * 5; // refresh every 5 seconds
uint16_t devCountPerMinute[60] = {0};
uint16_t devCountPerMinutePerPeriod[HEAPMAP_BUFFLEN] = {0};
uint8_t devCountPerMinuteIndex = 0;
uint8_t devCountPerMinutePerPeriodIndex = 0;
bool devCountWasUpdated = false;
uint16_t maxcdpm = 0;
uint16_t mincdpm = 0xffff;

uint16_t maxcdpmpp = 0;
uint16_t mincdpmpp = 0xffff;


static bool blinkit = false; // task blinker state
static bool blinktoggler = true;
static bool appIconRendered = false;
static bool earthIconRendered = false;
static bool foundTimeServer = false;
static bool foundFileServer = false;
static bool gpsIconVisible = false;
static bool uptimeIconRendered = false;
static bool foundFileServerIconRendered = true;
static uint16_t BLEStateIconColor;
static uint16_t lastBLEStateIconColor;
static uint16_t dbIconColor;
static uint16_t lastdbIconColor;
static unsigned long blinknow = millis(); // task blinker start time
static unsigned long scanTime = SCAN_DURATION * 1000; // task blinker duration
static unsigned long blinkthen = blinknow + scanTime; // task blinker end time
static unsigned long lastblink = millis(); // task blinker last blink
static unsigned long lastprogress = millis(); // task blinker progress

const char* lastDevicesCountTpl = "Last:%s%2s";
const char* seenDevicesCountTpl = "Seen:%s%4s";
const char* scansCountTpl = "Scans:%s%4s";
const char* heapTpl = "Heap: %6d";
const char* entriesTpl = "Entries:%4s";
const char* addressTpl = "  %s";
const char* dbmTpl = "%ddBm    ";
const char* ouiTpl = "      %s";
const char* appearanceTpl = "  Appearance: %d";
const char* manufTpl = "      %s";
const char* nameTpl = "      %s";
const char* screenshotFilenameTpl = "/screenshot-%04d-%02d-%02d_%02dh%02dm%02ds.565";
const char* hitsTimeStampTpl = "      %04d/%02d/%02d %02d:%02d:%02d %s";

static char entriesStr[14] = {'\0'};
static char heapStr[16] = {'\0'};
static char sessDevicesCountStr[16] = {'\0'};
static char devicesCountStr[16] = {'\0'};
static char newDevicesCountStr[16] = {'\0'};
static char unitOutput[16] = {'\0'};
static char screenshotFilenameStr[42] = {'\0'};
static char addressStr[24] = {'\0'};
static char dbmStr[16] = {'\0'};
static char hitsTimeStampStr[48] = {'\0'};
static char hitsStr[16] = {'\0'};
static char nameStr[38] = {'\0'};
static char ouiStr[38] = {'\0'};
static char appearanceStr[48] = {'\0'};
static char manufStr[38] = {'\0'};

char *macAddressToColorStr = (char*)calloc(MAC_LEN+1, sizeof(char*));

// github avatar style mac address visual code generation \o/
// builds a 8x8 vertically symetrical matrix based on the
// bytes in the mac address, two first bytes are used to
// allocate a color, the four last bytes are drawn with
// that color
struct MacAddressColors {
  uint8_t MACBytes[8]; // 8x8
  uint16_t color;
  uint8_t scaleX, scaleY;
  size_t size;
  size_t choplevel = 0;
  MacAddressColors( const char* address, byte _scaleX, byte _scaleY ) {
    scaleX = _scaleX;
    scaleY = _scaleY;
    size = 8 * 8 * scaleX * scaleY;
    memcpy( macAddressToColorStr, address, MAC_LEN+1);
    uint8_t tokenpos = 0, val, i, j, macpos, msb, lsb;
    char *token;
    char *ptr;
    token = strtok(macAddressToColorStr, ":");
    while(token != NULL) {
      val = strtol(token, &ptr, 16);
      switch( tokenpos )  {
        case 0: msb = val; break;
        case 1: lsb = val; break;
        default:
          macpos = tokenpos-2;
          MACBytes[macpos] = val;
          MACBytes[7-macpos]= val;
        break;
      }
      tokenpos++;
      token = strtok(NULL, ":");
    }
    color = (msb*256) + lsb;
  }
  void chopDraw( int32_t posx, int32_t posy, uint16_t height ) {
    if( height%scaleY != 0 || height > scaleY * 8 ) { // not a multiple !!
      log_e("Bad height request, height %d must be a multiple of scaleY %d and inferior to sizeY %d", height, scaleY, scaleY*8 );
      return;
    }
    uint8_t amount = height / scaleY;
    if( choplevel + amount > 8 || amount <= 0 ) { // out of range
      log_e("Bad height request ( i=%d; i<%d; i++)", choplevel, choplevel+amount );
      return;
    }
    tft.startWrite();
    tft.setAddrWindow( posx, posy, scaleX * 8, height );
    for( uint8_t i = choplevel; i < choplevel + amount; i++ ) {
      for( uint8_t sy = 0; sy < scaleY; sy++ ) {
        for( uint8_t j = 0; j < 8; j++ ) {
          if( bitRead( MACBytes[j], i ) == 1 ) {
            tft.pushColor( color, scaleX );
          } else {
            tft.pushColor( BLE_WHITE, scaleX );
          }
        }
      }
    }
    tft.endWrite();
    choplevel += amount;
  }
};


static char *formatUnit( int64_t number ) {
  *unitOutput = {'\0'};
  if( number > 999999 ) {
    sprintf(unitOutput, "%dM", number/1000000);
  } else if( number > 999 ) {
    sprintf(unitOutput, "%dK", number/1000);
  } else {
    sprintf(unitOutput, "%d", number);
  }
  return unitOutput;
}



class UIUtils {
  public:

    bool filterVendors = false;
    bool ScreenShotLoaded = false;
    byte brightness = BASE_BRIGHTNESS; // multiple of 8 otherwise can't turn off ^^
    byte brightnessIncrement = 8;

    struct BLECardStyle {
      uint16_t textColor = BLE_WHITE;
      uint16_t borderColor = BLE_WHITE;
      uint16_t bgColor = BLECARD_BGCOLOR;
      void setTheme( BLECardThemes themeID ) {
        //bgColor = BLECARD_BGCOLOR;
        switch ( themeID ) {
          case IN_CACHE_ANON:         borderColor = IN_CACHE_COLOR;     textColor = ANONYMOUS_COLOR;     break; // = 0,
          case IN_CACHE_NOT_ANON:     borderColor = IN_CACHE_COLOR;     textColor = NOT_ANONYMOUS_COLOR; break; // = 1,
          case NOT_IN_CACHE_ANON:     borderColor = NOT_IN_CACHE_COLOR; textColor = ANONYMOUS_COLOR;     break; // = 2,
          case NOT_IN_CACHE_NOT_ANON: borderColor = NOT_IN_CACHE_COLOR; textColor = NOT_ANONYMOUS_COLOR; break; // = 3
        }
        bgColor = textColor; // force transparency
      }
    };

    BLECardStyle BLECardTheme;

    void init() {
      Serial.begin(115200);
      Serial.println(welcomeMessage);
      Serial.printf("RTC_PROFILE: %s\nHAS_EXTERNAL_RTC: %s\nHAS_GPS: %s\nTIME_UPDATE_SOURCE: %d\n",
        RTC_PROFILE,
        HAS_EXTERNAL_RTC ? "true" : "false",
        HAS_GPS ? "true" : "false",
        TIME_UPDATE_SOURCE
      );
      Serial.println("Free heap at boot: " + String(initial_free_heap));

      bool clearScreen = true;
      if (resetReason == 12) { // SW Reset
        clearScreen = false;
      }

      tft_begin();
      tft_initOrientation(); //tft.setRotation( 0 ); // required to get smooth scrolling
      tft_setBrightness( brightness );
      setUISizePos(); // set position/dimensions for widgets and other UI items

      RGBColor colorstart = { 0x44, 0x44, 0x88 };
      RGBColor colorend   = { 0x22, 0x22, 0x44 };

      if (clearScreen) {
        tft.fillScreen(BLE_BLACK);
        tft_fillGradientHRect( 0, headerHeight, Out.width/2, scrollHeight, colorstart, colorend );
        tft_fillGradientHRect( Out.width/2, headerHeight, Out.width/2, scrollHeight, colorend, colorstart );
        // clear heap map
        for (uint16_t i = 0; i < HEAPMAP_BUFFLEN; i++) heapmap[i] = 0;
      }
      // fill header
      tft.fillRect(0, 0, Out.width, headerHeight, HEADER_BGCOLOR);
      // fill footer
      tft.fillRect(0, footerBottomPosY - footerHeight, Out.width, footerHeight, FOOTER_BGCOLOR);
      // fill progressbar
      tft.fillRect(0, progressBarY, Out.width, 2, BLE_GREENYELLOW);
      // fill bottom decoration
      if( footerHeight > 16 ) { // landscape mode does not need this decoration
        tft.fillRect(0, footerBottomPosY - (footerHeight - 2), Out.width, 1, BLE_DARKGREY);
      }

      AmigaBall.init();
      tft_drawJpg( BLECollector_Title_jpg, BLECollector_Title_jpg_len, 2, 3, 82,  8);

      if (resetReason == 12) { // SW Reset
        headerStats("Rebooted");
      } else {
        headerStats("Init UI");
      }
      Out.setupScrollArea( headerHeight, footerHeight, colorstart, colorend );

      SDSetup();
      timeSetup();
      timeStateIcon();
      footerStats();
      cacheStats();
      if ( clearScreen ) {
        playIntro();
      } else {
        Out.scrollNextPage();
      }
    }

    void begin() {
      xTaskCreatePinnedToCore(taskHeapGraph, "taskHeapGraph", 1024, NULL, 0, NULL, 1);
    }

    
    void update() {
      if ( freeheap + heap_tolerance < min_free_heap ) {
        headerStats("Out of heap..!");
        log_e("[FATAL] Heap too low: %d", freeheap);
        delay(1000);
        ESP.restart();
      }
      headerStats();
      footerStats();
    }


    void playIntro() {
      takeMuxSemaphore();
      uint16_t pos = 0;

      for (int i = 0; i < 5; i++) {
        pos += Out.println();
      }
      const char* introTextTitle = PLATFORM_NAME " BLE Collector";
      tft_getTextBounds(introTextTitle, Out.scrollPosX, Out.scrollPosY, &Out.x1_tmp, &Out.y1_tmp, &Out.w_tmp, &Out.h_tmp);
      uint16_t boxWidth = Out.w_tmp + 24;
      pos += Out.println();
      alignTextAt( introTextTitle, 6, Out.scrollPosY-Out.h_tmp, BLE_GREENYELLOW, BLE_TRANSPARENT/*BLECARD_BGCOLOR*/, ALIGN_CENTER );
      pos += Out.println();
      pos += Out.println();
      alignTextAt( "(c+)  tobozo  2019", 6, Out.scrollPosY-Out.h_tmp, BLE_GREENYELLOW, BLE_TRANSPARENT, ALIGN_CENTER );
      //pos += Out.println();
      pos += Out.println();
      pos += Out.println();
      tft_drawJpg( tbz_28x28_jpg, tbz_28x28_jpg_len, (Out.width/2 - 14), Out.scrollPosY - pos + 8, 28,  28);
      Out.drawScrollableRoundRect( (Out.width/2 - boxWidth/2), Out.scrollPosY-pos, boxWidth, pos, 8, BLE_GREENYELLOW );

      for (int i = 0; i < 5; i++) {
        Out.println();
      }

      giveMuxSemaphore();
      delay(2000);
      takeMuxSemaphore();
      Out.scrollNextPage();
      giveMuxSemaphore();
      #ifndef SKIP_INTRO
      xTaskCreatePinnedToCore(introUntilScroll, "introUntilScroll", 2048, NULL, 1, NULL, 1);
      #endif
    }


    static void screenShot() {

      takeMuxSemaphore();
      isQuerying = true;
      M5.ScreenShot.snap("BLECollector", true);
      isQuerying = false;
      giveMuxSemaphore();

    }

    static void screenShow( void * fileName = NULL ) {
      if( fileName == NULL ) return;
      isQuerying = true;

      if( String( (const char*)fileName ).endsWith(".jpg" ) ) {
        if( !BLE_FS.exists( (const char*)fileName ) ) {
          log_e("File %s does not exist\n", (const char*)fileName );
          isQuerying = false;
          return;
        }
        takeMuxSemaphore();
        Out.scrollNextPage(); // reset scroll position to zero otherwise image will have offset
        tft.drawJpgFile( BLE_FS, (const char*)fileName, 0, 0, Out.width, Out.height, 0, 0, JPEG_DIV_NONE );
        giveMuxSemaphore();
        vTaskDelay( 5000 );
        return;
      }
      if( String( (const char*)fileName ).endsWith(".565" ) ) {
        File screenshotFile = BLE_FS.open( (const char*)fileName );
        if(!screenshotFile) {
          Serial.printf("Failed to open file %s\n", (const char*)fileName );
          screenshotFile.close();
          return;
        }
        takeMuxSemaphore();
        Out.scrollNextPage(); // reset scroll position to zero otherwise image will have offset
        uint16_t imgBuffer[320]; // one scan line used for screen capture
        for(uint16_t y=0; y<Out.height; y++) {
          screenshotFile.read( (uint8_t*)imgBuffer, sizeof(uint16_t)*Out.width );
          tft_drawBitmap(0, y, Out.width, 1, imgBuffer);
        }
        giveMuxSemaphore();
      }
      isQuerying = false;
    }


    static void introUntilScroll( void * param ) {
      int initialscrollPosY = Out.scrollPosY;
      // animate until the scroll is called
      while(initialscrollPosY == Out.scrollPosY) {
        takeMuxSemaphore();
        AmigaBall.animate(1, false);
        giveMuxSemaphore();
        delay(1);
      }
      vTaskDelete(NULL);
    }


    static void headerStats(const char *status = "") {
      if ( isInScroll() || isInQuery() ) return;
      takeMuxSemaphore();
      int16_t posX = tft.getCursorX();
      int16_t posY = tft.getCursorY();
      int16_t statuspos = 0;
      *heapStr = {'\0'};
      *entriesStr = {'\0'};
      if ( !isEmpty( status ) ) {
        uint8_t alignoffset = leftMargin;
        tft.fillRect(0, headerStatsIconsY + headerLineHeight, iconAppX, 8, HEADER_BGCOLOR); // clear whole message status area
        if (strstr(status, "Inserted")) {
          tft_drawJpg(disk_jpeg, disk_jpeg_len, alignoffset,   headerStatsIconsY + headerLineHeight, 8, 8 ); // disk icon
          alignoffset +=10;
        } else if (strstr(status, "Cache")) {
          tft_drawJpg(ghost_jpeg, ghost_jpeg_len, alignoffset, headerStatsIconsY + headerLineHeight, 8, 8 ); // ghost icon
          alignoffset +=10;
        } else if (strstr(status, "DB")) {
          tft_drawJpg(moai_jpeg, moai_jpeg_len, alignoffset,   headerStatsIconsY + headerLineHeight, 8, 8 ); // moai icon
          alignoffset +=10;
        } else if (strstr(status, "Scan")) {
          tft_drawJpg( zzz_jpeg, zzz_jpeg_len, alignoffset,    headerStatsIconsY + headerLineHeight, 8,  8 ); // sleep icon
          alignoffset +=10;
        }
        alignTextAt( status, alignoffset, headerStatsIconsY + headerLineHeight, BLE_YELLOW, HEADER_BGCOLOR, ALIGN_FREE );
        statuspos = Out.x1_tmp + Out.w_tmp;
      }

      if( !showScanStats ) {
        showHeap    = scan_rounds%5==0;
        showEntries = scan_rounds%5==1;
      }

      if( showHeap ) {
        sprintf(heapStr, heapTpl, freeheap);
        alignTextAt( heapStr, heapStrX, heapStrY, BLE_GREENYELLOW, HEADER_BGCOLOR, heapAlign );
      }
      if( showEntries ) {
        sprintf(entriesStr, entriesTpl, formatUnit(entries));
        alignTextAt( entriesStr, entriesStrX, entriesStrY, BLE_GREENYELLOW, HEADER_BGCOLOR, entriesAlign );
      }

      if( !appIconRendered || statuspos > iconAppX ) { // only draw if text has overlapped
        tft_drawJpg( tbz_28x28_jpg, tbz_28x28_jpg_len, iconAppX, iconAppY, 28,  28); // app icon
        appIconRendered = true;
      }
      if( !earthIconRendered ) {
        tft_drawJpg(earth_jpeg, earth_jpeg_len, headerStatsIconsX, headerStatsIconsY + headerLineHeight, 8, 8); // entries icon
        earthIconRendered = true;
      }

      if( foundFileServer ) {
        if( !foundFileServerIconRendered ) {
          tft.fillRect( headerStatsIconsX, headerStatsIconsY, 8, 8, BLE_GREENYELLOW);
          tft.drawRect( headerStatsIconsX, headerStatsIconsY, 8, 8, BLE_DARKGREY);
          foundFileServerIconRendered = true;
        }
      } else {
        if( foundFileServerIconRendered ) {
          tft_drawJpg(ram_jpeg,     ram_jpeg_len, headerStatsIconsX, headerStatsIconsY,      8, 8); // heap icon
          foundFileServerIconRendered = false;
        }
      }

      tft.setCursor(posX, posY);
      giveMuxSemaphore();
    }


    void footerStats() {
      if ( isInScroll() || isInQuery() ) return;
      takeMuxSemaphore();
      int16_t posX = tft.getCursorX();
      int16_t posY = tft.getCursorY();

      if( TimeIsSet ) {
        alignTextAt( hhmmString, hhmmPosX, hhmmPosY, BLE_YELLOW, FOOTER_BGCOLOR, ALIGN_FREE );
      }
      alignTextAt( UpTimeString, uptimePosX, uptimePosY, BLE_GREENYELLOW, FOOTER_BGCOLOR, ALIGN_FREE );

      if( !uptimeIconRendered) {
        uptimeIconRendered = true; // only draw once
        tft_drawJpg( uptime_jpg, uptime_jpg_len, uptimePosX - 16, uptimePosY - 4, 12,  12);
      }
      alignTextAt("(c+) tobozo", copyleftPosX, copyleftPosY, BLE_YELLOW, FOOTER_BGCOLOR, ALIGN_FREE );
      #if HAS_GPS
        if( gpsIconVisible ) {
          tft_drawJpg( gps_jpg, gps_jpg_len, hhmmPosX + 31, hhmmPosY - 2, 10,  10);
        } else {
          tft.fillRect( hhmmPosX + 31, hhmmPosY - 2, 10,  10, FOOTER_BGCOLOR );
        }
      #endif

      if( !showScanStats ) {
        showCdevc = scan_rounds%5==2;
        showSessc = scan_rounds%5==3;
        showNdevc = scan_rounds%5==4;
      }

      if( showCdevc ) {
        *devicesCountStr = {'\0'};
        sprintf( devicesCountStr, lastDevicesCountTpl, lastDevicesCountSpacer, formatUnit(devicesCount) );
        alignTextAt( devicesCountStr, cdevcPosX, cdevcPosY, BLE_GREENYELLOW, FOOTER_BGCOLOR, cdevcAlign );
      }
      if( showSessc ) {
        *sessDevicesCountStr = {'\0'};
        sprintf( sessDevicesCountStr, seenDevicesCountTpl, seenDevicesCountSpacer, formatUnit(sessDevicesCount) );
        alignTextAt( sessDevicesCountStr, sesscPosX, sesscPosY, BLE_YELLOW, FOOTER_BGCOLOR, sesscAlign );
      }
      if( showNdevc ) {
        *newDevicesCountStr = {'\0'};
        sprintf( newDevicesCountStr, scansCountTpl, scansCountSpacer, formatUnit(scan_rounds) );
        alignTextAt( newDevicesCountStr, ndevcPosX, ndevcPosY, BLE_GREENYELLOW, FOOTER_BGCOLOR, ndevcAlign );
      }

      tft.setCursor(posX, posY);
      giveMuxSemaphore();
    }


    void cacheStats() {
      takeMuxSemaphore();
      percentBox( percentBoxX, percentBoxY - 3*(percentBoxSize+2), percentBoxSize, percentBoxSize, BLEDevCacheUsed, BLE_CYAN,        BLE_BLACK);
      percentBox( percentBoxX, percentBoxY - 2*(percentBoxSize+2), percentBoxSize, percentBoxSize, VendorCacheUsed, BLE_ORANGE,      BLE_BLACK);
      percentBox( percentBoxX, percentBoxY - 1*(percentBoxSize+2), percentBoxSize, percentBoxSize, OuiCacheUsed,    BLE_GREENYELLOW, BLE_BLACK);
      if( filterVendors ) {
        // TODO: landscape this
        tft_drawJpg( filter_jpeg, filter_jpeg_len, filterVendorsIconX, filterVendorsIconY, 10,  8);
      } else {
        tft.fillRect( filterVendorsIconX, filterVendorsIconY, 10,  8, FOOTER_BGCOLOR );
      }
      giveMuxSemaphore();
    }


    void percentBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t percent, uint16_t barcolor, uint16_t bgcolor, uint16_t bordercolor = BLE_DARKGREY) {
      if (percent == 0) {
        tft.drawRect(x - 1, y - 1, w + 2, h + 2, bordercolor);
        tft.fillRect(x, y, w, h, bgcolor);
        return;
      }
      if (percent == 100) {
        tft.drawRect(x - 1, y - 1, w + 2, h + 2, BLUETOOTH_COLOR);
        tft.fillRect(x, y, w, h, barcolor);
        return;
      }
      float ratio = 10.0;
      if( w == h ) {
        ratio = w;
      } else {
        ratio = (float)w / (float)h * (float)10.0;
      }
      tft.drawRect(x - 1, y - 1, w + 2, h + 2, bordercolor);
      tft.fillRect(x, y, w, h, bgcolor);
      uint8_t yoffsetpercent = percent / ratio;
      uint8_t boxh = (yoffsetpercent * h) / ratio ;
      tft.fillRect(x, y, w, boxh, barcolor);

      uint8_t xoffsetpercent = percent % (int)ratio;
      if (xoffsetpercent == 0) return;
      uint8_t linew = (xoffsetpercent * w) / ratio;
      tft.drawFastHLine(x, y + boxh, linew, barcolor);
    }


    static void timeStateIcon() {
      tft_drawJpg( clock_jpeg, clock_jpeg_len, iconRtcX-iconR, iconRtcY-iconR+1, 8,  8);
      //tft.fillCircle(iconRtcX, iconRtcY, iconR, BLE_GREENYELLOW);
      if (RTCisRunning) {
        //tft.drawCircle(iconRtcX, iconRtcY, iconR, BLE_DARKGREEN);
        //tft.drawFastHLine(iconRtcX, iconRtcY, iconR, BLE_DARKGREY);
        //tft.drawFastVLine(iconRtcX, iconRtcY, iconR - 2, BLE_DARKGREY);
      } else {
        tft.drawCircle(iconRtcX, iconRtcY, iconR, BLE_RED);
        //tft.drawFastHLine(iconRtcX, iconRtcY, iconR, BLE_RED);
        //tft.drawFastVLine(iconRtcX, iconRtcY, iconR - 2, BLE_RED);
      }
    }


    static void startBlink() { // runs one and detaches
      blinkit = true;
      blinknow = millis();
      scanTime = SCAN_DURATION * 1000;
      blinkthen = blinknow + scanTime;
      lastblink = millis();
      lastprogress = millis();
    }


    static void stopBlink() {
      // clear progress bar
      blinkit = false;
      delay(150); // give some time to the task to end
    }

    // sqlite state (read/write/inert) icon
    static void DBStateIconSetColor(int state) {
      switch (state) {
        case 2:/*DB OPEN FOR WRITING*/ dbIconColor = BLE_ORANGE;    break;
        case 1:/*DB_OPEN FOR READING*/ dbIconColor = BLE_YELLOW;    break;
        case 0:/*DB CLOSED*/           dbIconColor = BLE_DARKGREEN; break;
        case -1:/*DB BROKEN*/          dbIconColor = BLE_RED;       break;
        default:/*DB INACTIVE*/        dbIconColor = BLE_DARKGREY;  break;
      }
    }

    static void BLEStateIconSetColor(uint16_t color) {
      BLEStateIconColor = color;
    }

    static void PrintFatalError( const char* message, uint16_t yPos = AMIGABALL_YPOS ) {
      alignTextAt( message, 0, yPos, BLE_YELLOW, BLECARD_BGCOLOR, ALIGN_CENTER );
    }

    static void PrintProgressBar(uint16_t width) {
      if( width > Out.width || width == 0 ) { // clear
        tft.fillRect(0, progressBarY, Out.width, 2, BLE_DARKGREY);
      } else {
        tft.fillRect(0,     progressBarY, width,           2, BLUETOOTH_COLOR);
        tft.fillRect(width, progressBarY, Out.width-width, 2, BLE_DARKGREY);
      }
    }


    static void PrintDBStateIcon() {
      if( lastdbIconColor != dbIconColor ) {
        log_v("blinked at %d", dbIconColor);
        takeMuxSemaphore();
        tft.fillCircle(iconDbX, iconDbY, iconR, dbIconColor);
        giveMuxSemaphore();
        lastdbIconColor = dbIconColor;
      }
    }


    static void PrintBLEStateIcon(bool fill = true) {
      if( lastBLEStateIconColor != BLEStateIconColor ) {
        takeMuxSemaphore();
        lastBLEStateIconColor = BLEStateIconColor;
        if (fill) {
          tft.fillCircle(iconBleX, iconBleY, iconR, BLEStateIconColor);
          if( foundTimeServer ) {
            tft.drawCircle(iconRtcX, iconRtcY, iconR, BLE_GREEN);
          }
        } else {
          if( foundTimeServer ) {
            tft.drawCircle(iconRtcX, iconRtcY, iconR, BLE_ORANGE);
          }
          tft.fillCircle(iconBleX, iconBleY, iconR - 1, BLEStateIconColor);
        }
        if( !foundTimeServer ) {
          tft.drawCircle(iconRtcX, iconRtcY, iconR, HEADER_BGCOLOR);
          tft_drawJpg( clock_jpeg, clock_jpeg_len, iconRtcX-iconR, iconRtcY-iconR+1, 8,  8);
        }
        giveMuxSemaphore();
      }
    }


    static void PrintBleScanWidgets() {
      if (!blinkit || blinknow >= blinkthen) {
        blinkit = false;
        if(BLEStateIconColor!=BLE_DARKGREY) {
          takeMuxSemaphore();
          //tft.fillRect(0, progressBarY, Out.width, 2, BLE_DARKGREY);
          PrintProgressBar( Out.width );
          giveMuxSemaphore();
          // clear blue pin
          BLEStateIconSetColor(BLE_DARKGREY);
        }
        return;
      }

      blinknow = millis();
      if (lastblink + random(222, 666) < blinknow) {
        blinktoggler = !blinktoggler;
        if (blinktoggler) {
          BLEStateIconSetColor(BLUETOOTH_COLOR);
        } else {
          BLEStateIconSetColor(HEADER_BGCOLOR);
        }
        lastblink = blinknow;
      }

      if (lastprogress + 1000 < blinknow) {
        unsigned long remaining = blinkthen - blinknow;
        int percent = 100 - ( ( remaining * 100 ) / scanTime );
        takeMuxSemaphore();
        PrintProgressBar( (Out.width * percent) / 100 );
        //tft.fillRect(0, progressBarY, (Out.width * percent) / 100, 2, BLUETOOTH_COLOR);
        giveMuxSemaphore();
        lastprogress = blinknow;
      }
    }

    // spawn subtasks and leave
    static void taskHeapGraph( void * pvParameters ) { // always running
      mux = xSemaphoreCreateMutex();
      xTaskCreatePinnedToCore(heapGraph, "HeapGraph", 4096, NULL, 4, NULL, 0); /* last = Task Core */
      xTaskCreatePinnedToCore(clockSync, "clockSync", 2048, NULL, 4, NULL, 1); // RTC wants to run on core 1 or it fails
      vTaskDelete(NULL);
    }


    static void clockSync(void * parameter) {

      TickType_t lastWaketime;
      lastWaketime = xTaskGetTickCount();
      devGraphFirstStatTime = millis();

      while(1) {

        if( TimeIsSet ) {
          takeMuxSemaphore();
          timeHousekeeping();
          giveMuxSemaphore();
        } else {
          uptimeSet();
        }

        devGraphStartedSince = millis() - devGraphFirstStatTime;
        devCountPerMinuteIndex = int( devGraphStartedSince / devGraphPeriodShort )%60;
        devCountPerMinute[devCountPerMinuteIndex] = devicesStatCount;
        devicesStatCount = 0;

        mincdpm = 0xffff;
        maxcdpm = 0;
        for ( uint8_t i = 0; i < 60; i++ ) {
          if( devCountPerMinute[i] > maxcdpm ) {
            maxcdpm = devCountPerMinute[i];
          }
          if( devCountPerMinute[i] != 0 && devCountPerMinute[i] < mincdpm ) {
            mincdpm = devCountPerMinute[i];
          }
        }

        if( devGraphStartedSince > devGraphPeriodLong && devCountPerMinuteIndex%(devGraphPeriodLong/1000) == 0 ) { // every 2s
          // 1mn of data => calc devices per minute per minute
          size_t totalCount = 0;
          for( uint8_t m=0; m<60; m++ ) {
            totalCount += devCountPerMinute[m];
          }
          devCountPerMinutePerPeriodIndex = int( devGraphStartedSince / devGraphPeriodLong ) % graphLineWidth;
          devCountPerMinutePerPeriod[devCountPerMinutePerPeriodIndex] = totalCount;
          mincdpmpp = 0xffff;
          maxcdpmpp = 0;
          for ( uint8_t i = 0; i < 60; i++ ) {
            if( devCountPerMinutePerPeriod[i] > maxcdpmpp ) {
              maxcdpmpp = devCountPerMinutePerPeriod[i];
            }
            if( devCountPerMinutePerPeriod[i] != 0 && devCountPerMinutePerPeriod[i] < mincdpmpp ) {
              mincdpmpp = devCountPerMinutePerPeriod[i];
            }
          }
          log_i( "%d devices per minute per %d seconds  (%.2f), min(%d), max(%d)", totalCount, (devGraphPeriodLong/1000), (float)totalCount/60, mincdpmpp, maxcdpmpp );
        }
        devCountWasUpdated = true;

        #if HAS_GPS
          if( GPSHasDateTime ) {
            gpsIconVisible = true;          
          } else {
            gpsIconVisible = false;
          }
        #endif
        // make sure it happens exactly every 1000 ms has
        vTaskDelayUntil(&lastWaketime, 1000 / portTICK_PERIOD_MS);
      }
    }


    static void heapGraph(void * parameter) {
      uint32_t lastfreeheap;
      uint32_t toleranceheap = min_free_heap + heap_tolerance;
      uint8_t i = 0;


      const uint16_t baseCoordY = /*graphY +*/ graphLineHeight-2; // set Y axis to 2px-bottom of the graph
      uint16_t dcpmFirstY = baseCoordY;
      uint16_t dcpmLastX  = 0;
      uint16_t dcpmLastY  = 0;
      uint16_t dcpmppFirstY = baseCoordY;
      uint16_t dcpmppLastX  = 0;
      uint16_t dcpmppLastY  = 0;

      heapGraphSprite.setPsram( false );

      while (1) {

        if ( isInScroll() || isInQuery() ) {
          vTaskDelay( 10 );
          continue;
        }
        // do the blinky stuff
        PrintDBStateIcon();
        PrintBLEStateIcon();
        PrintBleScanWidgets();

        if( ! devCountWasUpdated ) {
          vTaskDelay( 10 );
          continue;
        }
        devCountWasUpdated = false;
        heapmap[heapindex++] = freeheap;
        heapindex = heapindex % HEAPMAP_BUFFLEN;
        lastfreeheap = freeheap;

        // render heatmap
        uint16_t GRAPH_COLOR = BLE_WHITE;
        uint32_t graphMin = min_free_heap;
        uint32_t graphMax = graphMin;
        uint32_t toleranceline = graphLineHeight;
        uint32_t minline = 0;
        uint16_t GRAPH_BG_COLOR = BLE_BLACK;
        // dynamic scaling
        for (i = 0; i < graphLineWidth; i++) {
          int thisindex = int(heapindex - graphLineWidth + i + HEAPMAP_BUFFLEN) % HEAPMAP_BUFFLEN;
          uint32_t heapval = heapmap[thisindex];
          if (heapval != 0 && heapval < graphMin) {
            graphMin =  heapval;
          }
          if (heapval > graphMax) {
            graphMax = heapval;
          }
        }

        if (graphMin == graphMax) {
          // data isn't relevant enough to render
          vTaskDelay( 100 );
          continue; 
        }
        // bounds, min and max lines
        minline = map(min_free_heap, graphMin, graphMax, 0, graphLineHeight);
        if (toleranceheap > graphMax) {
          GRAPH_BG_COLOR = BLE_ORANGE;
          toleranceline = graphLineHeight;
        } else if ( toleranceheap < graphMin ) {
          toleranceline = 0;
        } else {
          toleranceline = map(toleranceheap, graphMin, graphMax, 0, graphLineHeight);
        }
        // draw graph
        takeMuxSemaphore();
        
        heapGraphSprite.setColorDepth( 8 );
        heapGraphSprite.createSprite( graphLineWidth, graphLineHeight );

        for (i = 0; i < graphLineWidth; i++) {
          int thisindex = int(heapindex - graphLineWidth + i + HEAPMAP_BUFFLEN) % HEAPMAP_BUFFLEN;
          uint32_t heapval = heapmap[thisindex];
          if ( heapval > toleranceheap ) {
            // nominal, all green
            GRAPH_COLOR = BLE_GREEN;
            GRAPH_BG_COLOR = BLE_DARKGREY;
          } else {
            if ( heapval > min_free_heap ) {
              // in tolerance zone
              GRAPH_COLOR = BLE_YELLOW;
              GRAPH_BG_COLOR = BLE_DARKGREEN;
            } else {
              // under tolerance zone
              if (heapval > 0) {
                GRAPH_COLOR = BLE_RED;
                GRAPH_BG_COLOR = BLE_ORANGE;
              } else {
                // no record
                GRAPH_BG_COLOR = BLE_BLACK;
              }
            }
          }
          // fill background
          heapGraphSprite.drawFastVLine( i, 0, graphLineHeight, GRAPH_BG_COLOR );
          if ( heapval > 0 ) {
            uint32_t lineheight = map(heapval, graphMin, graphMax, 0, graphLineHeight);
            heapGraphSprite.drawFastVLine( i, graphLineHeight-lineheight, lineheight, GRAPH_COLOR );
          }
        }
        heapGraphSprite.drawFastHLine( 0, graphLineHeight - toleranceline, graphLineWidth, BLE_LIGHTGREY );
        heapGraphSprite.drawFastHLine( 0, graphLineHeight - minline, graphLineWidth, BLE_RED );

        if( devGraphStartedSince > devGraphPeriodLong ) {
          // 1mn of data => calc devices per minute
          size_t totalCount = 0;
          for( uint8_t m=0; m<60; m++ ) {
            totalCount += devCountPerMinute[m];
          }

          dcpmLastX  = 0;
          dcpmLastY  = dcpmFirstY;

          dcpmppLastX  = 0;
          dcpmppLastY  = 0;

          for (i = 0; i < graphLineWidth; i++) {
            uint16_t coordX = /*graphX +*/ i;
            uint16_t coordY = 0;

            uint8_t perMinuteIndex = map( (devCountPerMinuteIndex+i+1)%60, 0, graphLineWidth, 0, 60 ); // map to X
            int16_t dcpm = map( devCountPerMinute[perMinuteIndex], mincdpm, maxcdpm, 0, (graphLineHeight/4)-2); // map to Y
            coordY = baseCoordY - dcpm;
            if( devCountPerMinute[perMinuteIndex] > 0 ) {
              if( i==0 ) {
                dcpmFirstY = coordY;
              } else {
                heapGraphSprite.drawLine( coordX, coordY, dcpmLastX, dcpmLastY, BLE_DARKBLUE );
                dcpmLastX = coordX;
              }
              dcpmLastY = coordY;
            }

            uint8_t perMinutePerPeriodIndex = (devCountPerMinutePerPeriodIndex+i);
            perMinutePerPeriodIndex++;
            perMinutePerPeriodIndex = perMinutePerPeriodIndex%graphLineWidth; // map to X
            int16_t dcpmpp = map( devCountPerMinutePerPeriod[perMinutePerPeriodIndex], mincdpmpp, maxcdpmpp, (graphLineHeight/4)+2, graphLineHeight*.75); // map to Y
            coordY = baseCoordY - dcpmpp;
            if( devCountPerMinutePerPeriod[perMinutePerPeriodIndex] > 0 ) {
              if( dcpmppLastY > 0 ) {
                heapGraphSprite.drawLine( coordX, coordY, dcpmppLastX, dcpmppLastY, BLE_DARKBLUE );
              }
              dcpmppLastX = coordX;
              dcpmppLastY = coordY;
            }

          }
          // join last/first
          heapGraphSprite.drawLine( dcpmLastX, dcpmLastY, graphLineWidth, dcpmFirstY, BLE_DARKBLUE );
        }
        heapGraphSprite.pushSprite( graphX, graphY );
        heapGraphSprite.deleteSprite();
        giveMuxSemaphore();
        vTaskDelay( 100 );
      }
    }


    void printBLECard( BlueToothDevice *BleCard ) {
      // don't render if already on screen
      if( BLECardIsOnScreen( BleCard->address ) ) {
        log_d("%s is already on screen, skipping rendering", BleCard->address);
        return;
      }

      if ( isEmpty( BleCard->address ) ) {
        log_w("Cowardly refusing to render %d with an empty address", 0);
        return;
      }

      if( filterVendors ) {
        if(strcmp( BleCard->ouiname, "[random]")==0 ) {
          log_i("Filtering %s with random vendorname", BleCard->address);
          return;
        }
      }

      log_d("  [printBLECard] %s will be rendered", BleCard->address);

      takeMuxSemaphore();

      //MacScrollView
      uint16_t randomcolor = tft_color565( random(128, 255), random(128, 255), random(128, 255) );
      uint16_t blockHeight = 0;
      uint16_t hop;
      uint16_t initialPosY = Out.scrollPosY;
      MacAddressColors AvatarizedMAC( BleCard->address, macAddrColorsScaleX, macAddrColorsScaleY );

      *addressStr = {'\0'};
      sprintf( addressStr, addressTpl, BleCard->address );
      *dbmStr = {'\0'};
      sprintf( dbmStr, dbmTpl, BleCard->rssi );

      if( BleCard->in_db ) {
        if( BleCard->is_anonymous ) {
          BLECardTheme.setTheme( IN_CACHE_ANON );
        } else {
          BLECardTheme.setTheme( IN_CACHE_NOT_ANON );
        }
      } else {
        if( BleCard->is_anonymous ) {
          BLECardTheme.setTheme( NOT_IN_CACHE_ANON );
        } else {
          BLECardTheme.setTheme( NOT_IN_CACHE_ANON );
        }
      }
      tft.setTextColor( BLECardTheme.textColor, BLECardTheme.bgColor );

      BGCOLOR = BLECardTheme.bgColor;
      hop = Out.println( SPACE );
      blockHeight += hop;

      hop = Out.println( addressStr );
      blockHeight += hop;

      alignTextAt( dbmStr, 0, Out.scrollPosY - hop, BLECardTheme.textColor, BLECardTheme.bgColor, ALIGN_RIGHT );
      tft.setCursor( 0, Out.scrollPosY );
      drawRSSI( Out.width - 18, Out.scrollPosY - hop - 1, BleCard->rssi, BLECardTheme.textColor );
      if ( BleCard->in_db ) { // 'already seen this' icon
        tft_drawJpg( update_jpeg, update_jpeg_len, 138, Out.scrollPosY - hop, 8,  8);
      } else { // 'just inserted this' icon
        tft_drawJpg( insert_jpeg, insert_jpeg_len, 138, Out.scrollPosY - hop, 8,  8);
      }
      if ( !isEmpty( BleCard->uuid ) ) { // 'has service UUID' Icon
        tft_drawJpg( service_jpeg, service_jpeg_len, 128, Out.scrollPosY - hop, 8,  8);
      }

      switch( BleCard->hits ) {
        case 0:
          tft_drawJpg( ghost_jpeg, ghost_jpeg_len, 118, Out.scrollPosY - hop, 8,  8);
        break;
        case 1:
          tft_drawJpg( moai_jpeg, moai_jpeg_len, 118, Out.scrollPosY - hop, 8,  8);
        break;
        default:
          tft_drawJpg( disk_jpeg, disk_jpeg_len, 118, Out.scrollPosY - hop, 8,  8);
        break;
      }


      if( TimeIsSet ) {
        if ( BleCard->hits > 1 ) {
          *hitsStr = {'\0'};
          sprintf(hitsStr, "(%s hits)", formatUnit( BleCard->hits ) );

          if( BleCard->updated_at.unixtime() > 0 /* BleCard->created_at.year() > 1970 */) {
            unsigned long age_in_seconds = abs( BleCard->created_at.unixtime() - BleCard->updated_at.unixtime() );
            unsigned long age_in_minutes = age_in_seconds / 60;
            unsigned long age_in_hours   = age_in_minutes / 60;
            unsigned long seconds_since_boot = (millis() / 1000)+1;
            float freq = ((float)BleCard->hits / (float)scan_rounds+1) * ((float)age_in_seconds / (float)seconds_since_boot+1);
            /*
            Serial.printf(" C: %d, U:%d, (%d / %d) * (%d / %d) = ",
              BleCard->created_at.unixtime(),
              BleCard->updated_at.unixtime(),
              BleCard->hits,
              scan_rounds+1,
              age_in_seconds+1,
              millis() / 1000
            );
            Serial.println( freq * 1000 );*/
          }

          if( BleCard->created_at.year() > 1970 ) {
            blockHeight += Out.println(SPACE);
            *hitsTimeStampStr = {'\0'};
            sprintf(hitsTimeStampStr, hitsTimeStampTpl, 
              BleCard->created_at.year(),
              BleCard->created_at.month(),
              BleCard->created_at.day(),
              BleCard->created_at.hour(),
              BleCard->created_at.minute(),
              BleCard->created_at.second(),
              hitsStr
            );
            hop = Out.println( hitsTimeStampStr );
            blockHeight += hop;
            tft_drawJpg( clock_jpeg, clock_jpeg_len, 12, Out.scrollPosY - hop, 8,  8 );
          }
        }
      }

      if ( !isEmpty( BleCard->ouiname ) ) {
        blockHeight += Out.println( SPACE );
        *ouiStr = {'\0'};
        sprintf( ouiStr, ouiTpl, BleCard->ouiname );
        hop = Out.println( ouiStr );
        blockHeight += hop;
        if ( strstr( BleCard->ouiname, "Espressif" ) ) {
          tft_drawJpg( espressif_jpeg  , espressif_jpeg_len, 11, Out.scrollPosY - hop, 8, 8 );
        } else {
          tft_drawJpg( nic16_jpeg, nic16_jpeg_len, 10, Out.scrollPosY - hop, 13, 8 );
        }
      }

      bool jumpNext = true;
      if( macAddrColorsSizeY <= Out.h_tmp ) {
        AvatarizedMAC.chopDraw( macAddrColorsPosX, Out.scrollPosY - hop, macAddrColorsSizeY );
      } else {
        // chop-chop!
        int16_t sizeY = macAddrColorsSizeY;
        while( sizeY >= Out.h_tmp ) {
          sizeY -= Out.h_tmp;
          AvatarizedMAC.chopDraw( macAddrColorsPosX, Out.scrollPosY - hop, Out.h_tmp );
          if( sizeY >= Out.h_tmp ) {
            blockHeight += Out.println(SPACE);
          }
        }
        jumpNext = false;
      }
      if ( BleCard->appearance != 0 ) {
        if( jumpNext ) {
          blockHeight += Out.println(SPACE);
        } else {
          jumpNext = true;
        }
        *appearanceStr = {'\0'};
        sprintf( appearanceStr, appearanceTpl, BleCard->appearance );
        hop = Out.println( appearanceStr );
        blockHeight += hop;
      }
      if ( !isEmpty( BleCard->manufname ) ) {
        if( jumpNext ) {
          blockHeight += Out.println(SPACE);
        } else {
          jumpNext = true;
        }
        *manufStr = {'\0'};
        sprintf( manufStr, manufTpl, BleCard->manufname );
        hop = Out.println( manufStr );
        blockHeight += hop;
        if ( strstr( BleCard->manufname, "Apple" ) ) {
          tft_drawJpg( apple16_jpeg, apple16_jpeg_len, 12, Out.scrollPosY - hop, 8,  8 );
        } else if ( strstr( BleCard->manufname, "IBM" ) ) {
          tft_drawJpg( ibm8_jpg, ibm8_jpg_len, 10, Out.scrollPosY - hop, 20,  8);
        } else if ( strstr (BleCard->manufname, "Microsoft" ) ) {
          tft_drawJpg( crosoft_jpeg, crosoft_jpeg_len, 12, Out.scrollPosY - hop, 8,  8 );
        } else if ( strstr( BleCard->manufname, "Bose" ) ) {
          tft_drawJpg( speaker_icon_jpg, speaker_icon_jpg_len, 12, Out.scrollPosY - hop, 6,  8 );
        } else {
          tft_drawJpg( generic_jpeg, generic_jpeg_len, 12, Out.scrollPosY - hop, 8,  8 );
        }
      }
      if ( !isEmpty( BleCard->name ) ) {
        *nameStr = {'\0'};
        sprintf(nameStr, nameTpl, BleCard->name);
        if( jumpNext ) {
          blockHeight += Out.println(SPACE);
        } else {
          jumpNext = true;
        }
        hop = Out.println( nameStr );
        blockHeight += hop;
        tft_drawJpg( name_jpeg, name_jpeg_len, 12, Out.scrollPosY - hop, 7,  8);
      }
      hop = Out.println( SPACE) ;
      blockHeight += hop;
      uint16_t boxHeight = blockHeight-2;
      uint16_t boxWidth  = Out.width - 2;
      uint16_t boxPosY   = initialPosY + 1;
      Out.drawScrollableRoundRect( 1, boxPosY, boxWidth, boxHeight, 4, BLECardTheme.borderColor );
      lastPrintedMacIndex++;
      lastPrintedMacIndex = lastPrintedMacIndex % BLECARD_MAC_CACHE_SIZE;
      memcpy( MacScrollView[lastPrintedMacIndex].address, BleCard->address, MAC_LEN+1 );
      MacScrollView[lastPrintedMacIndex].blockHeight = blockHeight;
      MacScrollView[lastPrintedMacIndex].scrollPosY  = boxPosY;//Out.scrollPosY;
      MacScrollView[lastPrintedMacIndex].borderColor = BLECardTheme.borderColor;
      giveMuxSemaphore();
    }


    static bool BLECardIsOnScreen( const char* address ) {
      log_v("Checking if %s is visible onScreen", address);
      uint16_t card_index;
      int16_t offset = 0;
      for(uint16_t i = lastPrintedMacIndex+BLECARD_MAC_CACHE_SIZE; i>lastPrintedMacIndex; i--) {
        card_index = i%BLECARD_MAC_CACHE_SIZE;
        offset+=MacScrollView[card_index].blockHeight;
        if ( strcmp( address, MacScrollView[card_index].address ) == 0 ) {
          if( offset <= Out.yArea ) {
            highlighbBLECard( card_index, -offset );
            log_v("%s is onScreen", address);
            return true;
          } else {
            log_v("%s is in cache but NOT visible onScreen", address);
            return false;
          }
        }
      }
      log_v("%s is NOT in cache and NOT visible onScreen", address);
      return false;      
    }

    static void highlighbBLECard( uint16_t card_index, int16_t offset ) {
      if( card_index >= BLECARD_MAC_CACHE_SIZE) return; // bad value
      if( isEmpty( MacScrollView[card_index].address ) ) return; // empty slot
      int newYPos = Out.translate( Out.scrollPosY, offset );
      headerStats( MacScrollView[card_index].address );
      takeMuxSemaphore();
      uint16_t boxHeight = MacScrollView[card_index].blockHeight-2;
      uint16_t boxWidth  = Out.width - 2;
      uint16_t boxPosY   = newYPos + 1;
      for( int16_t color=255; color>64; color-- ) {
        Out.drawScrollableRoundRect( 1, boxPosY, boxWidth, boxHeight, 4, tft_color565(color, color, color) );
        delay(8); // TODO: use a timer
      }
      Out.drawScrollableRoundRect( 1, boxPosY, boxWidth, MacScrollView[card_index].blockHeight-2, 4, MacScrollView[card_index].borderColor );
      giveMuxSemaphore();
    }


  private:

    static void alignTextAt(const char* text, uint16_t x, uint16_t y, int16_t color = BLE_YELLOW, int16_t bgcolor = BLE_TRANSPARENT, uint8_t textAlign = ALIGN_FREE) {
      if( bgcolor != BLE_TRANSPARENT ) {
        tft.setTextColor( color, bgcolor );
      } else {
        tft.setTextColor( color, color ); // force transparency
      }
      tft_getTextBounds(text, x, y, &Out.x1_tmp, &Out.y1_tmp, &Out.w_tmp, &Out.h_tmp);
      switch (textAlign) {
        case ALIGN_FREE:
          tft.setCursor(x, y);
          break;
        case ALIGN_LEFT:
          tft.setCursor(0, y);
          break;
        case ALIGN_RIGHT:
          tft.setCursor(Out.width - Out.w_tmp, y);
          break;
        case ALIGN_CENTER:
          tft.setCursor(Out.width / 2 - Out.w_tmp / 2, y);
          break;
      }
      tft.drawString( text, tft.getCursorX(), tft.getCursorY() );
    }


    // draws a RSSI Bar for the BLECard
    void drawRSSI(int16_t x, int16_t y, int16_t rssi, uint16_t bgcolor) {
      uint16_t barColors[4];
      if (rssi >= -30) {
        // -30 dBm and more Amazing    - Max achievable signal strength. The client can only be a few feet
        // from the AP to achieve this. Not typical or desirable in the real world.  N/A
        barColors[0] = BLE_GREEN;
        barColors[1] = BLE_GREEN;
        barColors[2] = BLE_GREEN;
        barColors[3] = BLE_GREEN;
      } else if (rssi >= -67) {
        // between -67 dBm and 31 dBm  - Very Good   Minimum signal strength for applications that require
        // very reliable, timely delivery of data packets.   VoIP/VoWiFi, streaming video
        barColors[0] = BLE_GREEN;
        barColors[1] = BLE_GREEN;
        barColors[2] = BLE_GREEN;
        barColors[3] = bgcolor;
      } else if (rssi >= -70) {
        // between -70 dBm and -68 dBm - Okay  Minimum signal strength for reliable packet delivery.   Email, web
        barColors[0] = BLE_YELLOW;
        barColors[1] = BLE_YELLOW;
        barColors[2] = BLE_YELLOW;
        barColors[3] = bgcolor;
      } else if (rssi >= -80) {
        // between -80 dBm and -71 dBm - Not Good  Minimum signal strength for basic connectivity. Packet delivery may be unreliable.  N/A
        barColors[0] = BLE_YELLOW;
        barColors[1] = BLE_YELLOW;
        barColors[2] = bgcolor;
        barColors[3] = bgcolor;
      } else if (rssi >= -90) {
        // between -90 dBm and -81 dBm - Unusable  Approaching or drowning in the noise floor. Any functionality is highly unlikely.
        barColors[0] = BLE_RED;
        barColors[1] = bgcolor;
        barColors[2] = bgcolor;
        barColors[3] = bgcolor;
      }  else {
        // dude, this sucks
        barColors[0] = BLE_RED; // want: BLE_RAINBOW
        barColors[1] = bgcolor;
        barColors[2] = bgcolor;
        barColors[3] = bgcolor;
      }
      tft.fillRect(x,     y + 4, 2, 4, barColors[0]);
      tft.fillRect(x + 3, y + 3, 2, 5, barColors[1]);
      tft.fillRect(x + 6, y + 2, 2, 6, barColors[2]);
      tft.fillRect(x + 9, y + 1, 2, 7, barColors[3]);
    }




  static DisplayMode getDisplayMode() {
    if( tft.width() > tft.height() ) {
      return TFT_LANDSCAPE;
    }
    if( tft.width() < tft.height() ) {
      return TFT_PORTRAIT;
    }
    return TFT_SQUARE;
  }

  // landscape / portrait theme switcher
  static void setUISizePos() {

    iconAppX = 124;
    iconAppY = 0;
    iconRtcX = 92;
    iconRtcY = 7;
    iconBleX = 104;
    iconBleY = 7;
    iconDbX = 116;
    iconDbY = 7;
    iconR = 4; // BLE icon radius

    switch( getDisplayMode() ) {
      case TFT_LANDSCAPE:
        log_w("Using UI in landscape mode (w:%d, h:%d)", Out.width, Out.height);
        sprintf(lastDevicesCountSpacer, "%s", "");
        sprintf(seenDevicesCountSpacer, "%s", "");
        sprintf(scansCountSpacer, "%s", "");
        headerHeight        = 35; // Important: resulting scrollHeight must be a multiple of font height, default font height is 8px
        footerHeight        = 13; // Important: resulting scrollHeight must be a multiple of font height, default font height is 8px
        scrollHeight        = ( Out.height - ( headerHeight + footerHeight ));
        leftMargin          = 2;
        footerBottomPosY    = Out.height;
        headerStatsX        = Out.width - 80;
        graphLineWidth      = HEAPMAP_BUFFLEN - 1;
        graphLineHeight     = 29;
        graphX              = Out.width - (150);
        graphY              = 0; // footerBottomPosY - 37;// 283
        percentBoxX         = (graphX - 12); // percentbox is 10px wide + 2px margin and 2px border
        percentBoxY         = graphLineHeight+2;
        percentBoxSize      = 8;
        headerStatsIconsX   = Out.width - (80 + 6);
        headerStatsIconsY   = 4;
        headerLineHeight    = 16;
        progressBarY        = 32;
        hhmmPosX            = 180;
        hhmmPosY            = footerBottomPosY - 8;
        gpsIconPosX         = hhmmPosX + 31;
        gpsIconPosY         = hhmmPosY - 2;
        uptimePosX          = 218;
        uptimePosY          = footerBottomPosY - 8;
        uptimeIconRendered  = true; // never render uptime icon
        copyleftPosX        = 250;
        copyleftPosY        = footerBottomPosY - 8;
        cdevcPosX           = leftMargin;
        cdevcPosY           = footerBottomPosY - 8;
        cdevcAlign          = ALIGN_LEFT;
        sesscPosX           = 42;
        sesscPosY           = footerBottomPosY - 8;
        sesscAlign          = ALIGN_LEFT;
        ndevcPosX           = 100;
        ndevcPosY           = footerBottomPosY - 8;
        ndevcAlign          = ALIGN_LEFT;
        macAddrColorsScaleX = 4;
        macAddrColorsScaleY = 2;
        macAddrColorsSizeX  = 8 * macAddrColorsScaleX;
        macAddrColorsSizeY  = 8 * macAddrColorsScaleY;
        macAddrColorsSize   = macAddrColorsSizeX * macAddrColorsSizeY;
        macAddrColorsPosX   = Out.width - ( macAddrColorsSizeX + 6 );
        filterVendorsIconX  = 164;
        filterVendorsIconY  = footerBottomPosY - 9;
        showScanStats       = true;
        showHeap            = true;
        showEntries         = true;
        showCdevc           = true;
        showSessc           = true;
        showNdevc           = true;
        heapStrX            = headerStatsX;
        heapStrY            = headerStatsIconsY;
        heapAlign           = ALIGN_RIGHT;
        entriesStrX         = headerStatsX;
        entriesStrY         = headerStatsIconsY + headerLineHeight;
        entriesAlign        = ALIGN_RIGHT;
      break;
      case TFT_PORTRAIT:
        log_w("Using UI in portrait mode");
        sprintf(lastDevicesCountSpacer, "%s", "   "); // Last
        sprintf(seenDevicesCountSpacer, "%s", " "); // Seen
        sprintf(scansCountSpacer, "%s", ""); // Scans
        headerHeight        = 35; // Important: resulting scrollHeight must be a multiple of font height, default font height is 8px
        footerHeight        = 45; // Important: resulting scrollHeight must be a multiple of font height, default font height is 8px
        scrollHeight        = ( Out.height - ( headerHeight + footerHeight ));
        leftMargin          = 2;
        footerBottomPosY    = Out.height;
        headerStatsX        = Out.width-112;
        graphLineWidth      = HEAPMAP_BUFFLEN - 1;
        graphLineHeight     = 35;
        graphX              = Out.width - graphLineWidth - 2;
        graphY              = footerBottomPosY - 37;// 283
        percentBoxX         = (graphX - 14); // percentbox is 10px wide + 2px margin and 2px border
        percentBoxY         = footerBottomPosY;
        percentBoxSize      = 10;
        headerStatsIconsX   = 156;
        headerStatsIconsY   = 4;
        headerLineHeight    = 14;
        progressBarY        = 30;
        hhmmPosX            = 97;
        hhmmPosY            = footerBottomPosY - 28;
        gpsIconPosX         = hhmmPosX + 31;
        gpsIconPosY         = hhmmPosY - 2;
        uptimePosX          = 97;
        uptimePosY          = footerBottomPosY - 18;
        copyleftPosX        = 77;
        copyleftPosY        = footerBottomPosY - 8;
        cdevcPosX           = leftMargin;
        cdevcPosY           = footerBottomPosY - 28;
        cdevcAlign          = ALIGN_LEFT;
        sesscPosX           = leftMargin;
        sesscPosY           = footerBottomPosY - 18;
        sesscAlign          = ALIGN_LEFT;
        ndevcPosX           = leftMargin;
        ndevcPosY           = footerBottomPosY - 8;
        ndevcAlign          = ALIGN_LEFT;
        macAddrColorsScaleX = 4;
        macAddrColorsScaleY = 2;
        macAddrColorsSizeX  = 8 * macAddrColorsScaleX;
        macAddrColorsSizeY  = 8 * macAddrColorsScaleY;
        macAddrColorsPosX   = Out.width - ( macAddrColorsSizeX + 6 );
        filterVendorsIconX  = 152;
        filterVendorsIconY  = footerBottomPosY - 12;
        showScanStats       = true;
        showHeap            = true;
        showEntries         = true;
        showCdevc           = true;
        showSessc           = true;
        showNdevc           = true;
        heapStrX            = Out.width-80;
        heapStrY            = headerStatsIconsY;
        heapAlign           = ALIGN_RIGHT;
        entriesStrX         = Out.width-80;
        entriesStrY         = headerStatsIconsY + headerLineHeight;
        entriesAlign        = ALIGN_RIGHT;
      break;
      case TFT_SQUARE:
      default:
        log_w("Using UI in square/squeezed mode (w:%d, h:%d)", Out.width, Out.height);
        sprintf(lastDevicesCountSpacer, "%s", "     "); // Last
        sprintf(seenDevicesCountSpacer, "%s", "   "); // Seen
        sprintf(scansCountSpacer, "%s", "  "); // Scans
        headerHeight        = 35; // Important: resulting scrollHeight must be a multiple of font height, default font height is 8px
        footerHeight        = 13; // Important: resulting scrollHeight must be a multiple of font height, default font height is 8px
        scrollHeight        = ( Out.height - ( headerHeight + footerHeight ));
        leftMargin          = 2;
        footerBottomPosY    = Out.height;
        headerStatsX        = Out.width - 80;
        graphLineWidth      = HEAPMAP_BUFFLEN - 1;
        graphLineHeight     = 29;
        graphX              = Out.width - (graphLineWidth);
        graphY              = 0; // footerBottomPosY - 37;// 283
        percentBoxX         = (graphX - 12); // percentbox is 10px wide + 2px margin and 2px border
        percentBoxY         = graphLineHeight+2;
        percentBoxSize      = 8;
        headerStatsIconsX   = Out.width - (80 + 6);
        headerStatsIconsY   = 4;
        headerLineHeight    = 16;
        progressBarY        = 32;
        hhmmPosX            = leftMargin;
        hhmmPosY            = footerBottomPosY - 8;
        gpsIconPosX         = hhmmPosX + 31;
        gpsIconPosY         = hhmmPosY - 2;
        uptimePosX          = 50;
        uptimePosY          = footerBottomPosY - 8;
        uptimeIconRendered  = true; // never render uptime icon
        copyleftPosX        = 90;
        copyleftPosY        = footerBottomPosY - 8;
        cdevcPosX           = headerStatsX;
        cdevcPosY           = footerBottomPosY - 8;
        cdevcAlign          = ALIGN_RIGHT;
        sesscPosX           = headerStatsX;
        sesscPosY           = footerBottomPosY - 8;
        sesscAlign          = ALIGN_RIGHT;
        ndevcPosX           = headerStatsX;
        ndevcPosY           = footerBottomPosY - 8;
        ndevcAlign          = ALIGN_RIGHT;
        macAddrColorsScaleX = 4;
        macAddrColorsScaleY = 2;
        macAddrColorsSizeX  = 8 * macAddrColorsScaleX;
        macAddrColorsSizeY  = 8 * macAddrColorsScaleY;
        macAddrColorsSize   = macAddrColorsSizeX * macAddrColorsSizeY;
        macAddrColorsPosX   = Out.width - ( macAddrColorsSizeX + 6 );
        filterVendorsIconX  = 35;
        filterVendorsIconY  = footerBottomPosY - 9;
        showScanStats       = false;
        showHeap            = false;
        showEntries         = true;
        showCdevc           = false;
        showSessc           = false;
        showNdevc           = false;
        heapStrX            = headerStatsX;
        heapStrY            = footerBottomPosY - 8;
        heapAlign           = ALIGN_RIGHT;
        entriesStrX         = headerStatsX;
        entriesStrY         = footerBottomPosY - 8;
        entriesAlign        = ALIGN_RIGHT;
      break;
    }
  }

};


UIUtils UI;

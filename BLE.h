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

#define TICKS_TO_DELAY 1000




const char* processTemplateLong = "%s%d%s%d";
const char* processTemplateShort = "%s%d";
static char processMessage[20];

static bool onScanProcessed = true;
static bool onScanPopulated = true;
static bool onScanPropagated = true;
static bool onScanPostPopulated = true;
static bool onScanRendered = true;
static bool onScanDone = true;
static bool scanTaskRunning = false;
static bool scanTaskStopped = true;

extern size_t devicesStatCount;

static char* serialBuffer = NULL;
static char* tempBuffer = NULL;
#define SERIAL_BUFFER_SIZE 64

unsigned long lastheap = 0;
uint16_t lastscanduration = SCAN_DURATION;
char heapsign[5]; // unicode sign terminated
char scantimesign[5]; // unicode sign terminated
BLEScanResults bleresults;
BLEScan *pBLEScan;


static uint16_t processedDevicesCount = 0;
bool foundDeviceToggler = true;


enum AfterScanSteps {
  POPULATE  = 0,
  IFEXISTS  = 1,
  RENDER    = 2,
  PROPAGATE = 3
};


// work in progress: MAC blacklist/whitelist
const char MacList[3][MAC_LEN + 1] = {
  "aa:aa:aa:aa:aa:aa",
  "bb:bb:bb:bb:bb:bb",
  "cc:cc:cc:cc:cc:cc"
};


static bool AddressIsListed( const char* address ) {
  for ( byte i = 0; i < sizeof(MacList); i++ ) {
    if ( strcmp(address, MacList[i] ) == 0) {
      return true;
    }
  }
  return false;
}


static bool deviceHasPayload( BLEAdvertisedDevice advertisedDevice ) {
  if ( !advertisedDevice.haveServiceUUID() ) return false;
  if( advertisedDevice.isAdvertisingService( timeServiceUUID ) ) {
    log_i( "Found Time Server %s : %s", advertisedDevice.getAddress().toString().c_str(), advertisedDevice.getServiceUUID().toString().c_str() );
    timeServerBLEAddress = advertisedDevice.getAddress().toString();
    timeServerClientType = advertisedDevice.getAddressType();
    foundTimeServer = true;
    if ( foundTimeServer && (!TimeIsSet || ForceBleTime) ) {
      return true;
    }
  }
  if( advertisedDevice.isAdvertisingService( FileSharingServiceUUID ) ) {
    log_i( "Found File Server %s : %s", advertisedDevice.getAddress().toString().c_str(), advertisedDevice.getServiceUUID().toString().c_str() );
    foundFileServer = true;
    fileServerBLEAddress = advertisedDevice.getAddress().toString();
    fileServerClientType = advertisedDevice.getAddressType();
    if ( fileSharingEnabled ) {
      log_w("Ready to connect to file server %s", fileServerBLEAddress.c_str());
      return true;
    }
  }
  return false;
}



class FoundDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    void onResult( BLEAdvertisedDevice advertisedDevice ) {

      devicesStatCount++; // raw stats for heapgraph

      bool scanShouldStop =  deviceHasPayload( advertisedDevice );

      if ( onScanDone  ) return;

      if ( scan_cursor < MAX_DEVICES_PER_SCAN ) {
        log_i("will store advertisedDevice in cache #%d", scan_cursor);
        BLEDevHelper.store( BLEDevScanCache[scan_cursor], advertisedDevice );
        //bool is_random = strcmp( BLEDevScanCache[scan_cursor]->ouiname, "[random]" ) == 0;
        bool is_random = (BLEDevScanCache[scan_cursor]->addr_type == BLE_ADDR_TYPE_RANDOM || BLEDevScanCache[scan_cursor]->addr_type == BLE_ADDR_TYPE_RPA_RANDOM );
        //bool is_blacklisted = isBlackListed( BLEDevScanCache[scan_cursor]->address );
        if ( UI.filterVendors && is_random ) {
          //TODO: scan_cursor++
          log_i( "Filtering %s", BLEDevScanCache[scan_cursor]->address );
        } else {
          if ( DB.hasPsram ) {
            if ( !is_random ) {
              DB.getOUI( BLEDevScanCache[scan_cursor]->address, BLEDevScanCache[scan_cursor]->ouiname );
            }
            if ( BLEDevScanCache[scan_cursor]->manufid > -1 ) {
              DB.getVendor( BLEDevScanCache[scan_cursor]->manufid, BLEDevScanCache[scan_cursor]->manufname );
            }
            BLEDevScanCache[scan_cursor]->is_anonymous = BLEDevHelper.isAnonymous( BLEDevScanCache[scan_cursor] );
            log_i(  "  stored and populated #%02d : %s", scan_cursor, advertisedDevice.getName().c_str());
          } else {
            log_i(  "  stored #%02d : %s", scan_cursor, advertisedDevice.getName().c_str());
          }
          scan_cursor++;
          processedDevicesCount++;
        }
        if ( scan_cursor == MAX_DEVICES_PER_SCAN ) {
          onScanDone = true;
        }
      } else {
        onScanDone = true;
      }
      if ( onScanDone ) {
        advertisedDevice.getScan()->stop();
        scan_cursor = 0;
        if ( SCAN_DURATION - 1 >= MIN_SCAN_DURATION ) {
          SCAN_DURATION--;
        }
      }
      foundDeviceToggler = !foundDeviceToggler;
      if (foundDeviceToggler) {
        UI.BLEStateIconSetColor(BLE_GREEN);
      } else {
        UI.BLEStateIconSetColor(BLE_DARKGREEN);
      }
      if( scanShouldStop ) {
        advertisedDevice.getScan()->stop();
      }
    }
};

FoundDeviceCallbacks *FoundDeviceCallback;// = new FoundDeviceCallbacks(); // collect/store BLE data


struct SerialCallback {
  SerialCallback(void (*f)(void *) = 0, void *d = 0)
    : function(f), data(d) {}
  void (*function)(void *);
  void *data;
};

struct CommandTpl {
  const char* command;
  SerialCallback cb;
  const char* description;
};

CommandTpl* SerialCommands;
uint16_t Csize = 0;

struct ToggleTpl {
  const char *name;
  bool &flag;
};

ToggleTpl* TogglableProps;
uint16_t Tsize = 0;

//extern void OTA_over_BLE_Client(void);

class BLEScanUtils {

  public:

    void init() {

      BLEDevice::init( PLATFORM_NAME " BLE Collector");
      getPrefs(); // load prefs from NVS
      UI.init(); // launch all UI tasks
      if ( ! DB.init() ) { // mount DB
        log_e("Error with .db files (not found or corrupted), starting BLE File Sharing");
        startSerialTask();
        //UI.begin();
        takeMuxSemaphore();
        Out.scrollNextPage();
        Out.println();
        Out.scrollNextPage();
        UI.PrintFatalError( "[ERROR]: .db files not found" );
        //Out.println( "[ERROR]: .db files not found" );
        giveMuxSemaphore();
        setFileSharingServerOn( NULL );
        //startFileSharingServer();
        return;
      }
      WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
      startSerialTask();
      startScanCB();
      UI.begin();
    }
    #ifdef WITH_WIFI
    static void setWiFiSSID( void * param = NULL ) {
      if( param == NULL ) return;
      sprintf( WiFi_SSID, "%s", (const char*)param);
    }
    static void setWiFiPASS( void * param = NULL ) {
      if( param == NULL ) return;
      sprintf( WiFi_PASS, "%s", (const char*)param);
    }
    static void stopBLECB( void * param = NULL ) {
      stopScanCB();
      xTaskCreatePinnedToCore( stopBLE, "stopBLE", 8192, NULL, 5, NULL, 1 ); /* last = Task Core */
    }

    static void stopBLE( void * param = NULL ) {
      Serial.println(F("Shutting BT Down ..."));
      Serial.print("esp_bt_controller_disable");
      esp_bt_controller_disable() ;
      Serial.print("esp_bt_controller_deinit ");
      esp_bt_controller_deinit();
      Serial.print("BT Shutdown finished");
      WiFi.mode(WIFI_STA);
      Serial.println(WiFi.macAddress());
      if( String( WiFi_SSID ) !="" && String( WiFi_PASS ) !="" ) {
        WiFi.begin( WiFi_SSID, WiFi_PASS );
      } else {
        WiFi.begin();
      }
      while(WiFi.status() != WL_CONNECTED) {
        log_e("Not connected");
        delay(1000);
      }
      log_w("Connected!");
      Serial.print("Connected to ");
      Serial.println(WiFi_SSID);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("");
      vTaskDelete( NULL );
    }
    #endif

    static void startScanCB( void * param = NULL ) {

      while ( fileSharingServerTaskIsRunning ) {
        log_w("Waiting for BLE File Server to stop ...");
        vTaskDelay(1000);
      }

      while ( fileSharingClientTaskIsRunning ) {
        log_w("Waiting for BLE File Client to stop ...");
        vTaskDelay(1000);
      }
      /*
      while ( timeServerIsRunning ) {
        log_w("Waiting for BLE Time Server to stop ...");
        vTaskDelay(1000);
      }
      */

      if ( timeClientisRunning ) {
        log_w("Waiting for BLE Time Client to stop ...");
        vTaskDelay(1000);
      }

      BLEDevice::setMTU(100);
      if ( !scanTaskRunning ) {
        log_d("Starting scan" );
        xTaskCreatePinnedToCore( scanTask, "scanTask", 6144, NULL, 5, NULL, 1 ); /* last = Task Core */
        while ( scanTaskStopped ) {
          log_d("Waiting for scan to start...");
          vTaskDelay(1000);
        }
        Serial.println("Scan started...");
        UI.headerStats("Scan started...");
      }
    }
    static void stopScanCB( void * param = NULL) {
      if ( scanTaskRunning ) {
        log_d("Stopping scan" );
        scanTaskRunning = false;
        BLEDevice::getScan()->stop();
        while (!scanTaskStopped) {
          log_d("Waiting for scan to stop...");
          vTaskDelay(1000);
        }
        Serial.println("Scan stopped...");
        UI.headerStats("Scan stopped...");
      }
    }
    static void restartCB( void * param = NULL ) {
      stopScanCB();
      xTaskCreatePinnedToCore( doRestart, "doRestart", 8192, param, 5, NULL, 1 ); // last = Task Core
    }
    static void doRestart( void * param = NULL ) {
      // "restart now" command skips db replication
      if ( strcmp( "now", (const char*)param ) != 0 ) {
        DB.updateDBFromCache( BLEDevRAMCache, false, false );
      }
      ESP.restart();
    }

    static void setFileSharingServerOn( void * param ) {
      if ( ! fileDownloadingEnabled ) {
        fileDownloadingEnabled = true;
        xTaskCreatePinnedToCore( startFileSharingServer, "startFileSharingServer", 2048, param, 0, NULL, 0 ); // last = Task Core
      }
    }

    static void startFileSharingServer( void * param = NULL) {
      bool scanWasRunning = scanTaskRunning;
      if ( fileSharingServerTaskIsRunning ) {
        log_e("FileSharingClientTask already running!");
        vTaskDelete( NULL );
        return;
      }
      if ( scanTaskRunning ) stopScanCB();
      fileSharingServerTaskIsRunning = true;
      xTaskCreatePinnedToCore( FileSharingServerTask, "FileSharingServerTask", 12000, NULL, 5, NULL, 0 );
      if ( scanWasRunning ) {
        while ( fileSharingServerTaskIsRunning ) {
          vTaskDelay( 1000 );
        }
        log_w("Resuming operations after FileSharingServerTask");
        fileDownloadingEnabled = false;
        startScanCB();
      } else {
        log_w("FileSharingServerTask started with nothing to resume");
      }
      vTaskDelete( NULL );
    }

    static void setFileSharingClientOn( void * param ) {
      fileSharingEnabled = true;
    }

    static void startFileSharingClient( void * param ) {
      bool scanWasRunning = scanTaskRunning;
      fileSharingClientStarted = true;
      if ( scanTaskRunning ) stopScanCB();
      fileSharingClientTaskIsRunning = true;
      xTaskCreatePinnedToCore( FileSharingClientTask, "FileSharingClientTask", 12000, param, 5, NULL, 0 ); // last = Task Core
      if ( scanWasRunning ) {
        while ( fileSharingClientTaskIsRunning ) {
          vTaskDelay( 1000 );
        }
        log_w("Resuming operations after FileSharingClientTask");
        fileSharingEnabled = false;
        startScanCB();
      } else {
        log_w("FileSharingClientTask spawned with nothing to resume");
      }
      vTaskDelete( NULL );
    }

    static void setTimeClientOn( void * param ) {
      if( !timeClientisStarted ) {
        timeClientisStarted = true;
        xTaskCreatePinnedToCore( startTimeClient, "startTimeClient", 2560, param, 0, NULL, 0 ); // last = Task Core
      } else {
        log_w("startTimeClient already called, time is also about patience");
      }
    }

    static void startTimeClient( void * param ) {
      bool scanWasRunning = scanTaskRunning;
      ForceBleTime = false;
      while ( ! foundTimeServer ) {
        vTaskDelay( 100 );
      }
      if ( scanTaskRunning ) stopScanCB();
      xTaskCreatePinnedToCore( TimeClientTask, "TimeClientTask", 2560, NULL, 5, NULL, 0 ); // TimeClient task prefers core 0
      if ( scanWasRunning ) {
        while ( timeClientisRunning ) {
          vTaskDelay( 1000 );
        }
        log_w("Resuming operations after TimeClientTask");
        timeClientisStarted = false;
        //DB.maintain();
        startScanCB();
      } else {
        log_w("TimeClientTask started with nothing to resume");
      }
      vTaskDelete( NULL );
    }


    static void setTimeServerOn( void * param ) {
      if( !timeServerStarted ) {
        timeServerStarted = true;
        xTaskCreatePinnedToCore( startTimeServer, "startTimeServer", 2048, param, 0, NULL, 0 ); // last = Task Core
      }
    }

    static void startTimeServer( void * param ) {
      // timeServer runs forever
      timeServerIsRunning = true;
      xTaskCreatePinnedToCore( TimeServerTask, "TimeServerTask", 2048, NULL, 1, NULL, 1 ); // TimeServerTask prefers core 1
      log_w("TimeServerTask started");
      vTaskDelete( NULL );
    }

    static void setBrightnessCB( void * param = NULL ) {
      if( param != NULL ) {
        UI.brightness = atoi( (const char*) param );
      }
      takeMuxSemaphore();
      tft_setBrightness( UI.brightness );
      giveMuxSemaphore();
      setPrefs();
      log_w("Brightness is now at %d", UI.brightness);
    }

    static void resetCB( void * param = NULL ) {
      DB.needsReset = true;
      Serial.println("DB Scheduled for reset");
      stopScanCB();
      DB.maintain();
      delay(100);
      //startScanCB();
    }
    static void pruneCB( void * param = NULL ) {
      DB.needsPruning = true;
      Serial.println("DB Scheduled for pruning");
      stopScanCB();
      DB.maintain();
      startScanCB();
    }
    static void toggleFilterCB( void * param = NULL ) {
      UI.filterVendors = ! UI.filterVendors;
      UI.cacheStats(); // refresh icon
      setPrefs(); // save prefs
      Serial.printf("UI.filterVendors = %s\n", UI.filterVendors ? "true" : "false" );
    }
    static void startDumpCB( void * param = NULL ) {
      DBneedsReplication = true;
      bool scanWasRunning = scanTaskRunning;
      if ( scanTaskRunning ) stopScanCB();
      while ( DBneedsReplication ) {
        vTaskDelay(1000);
      }
      if ( scanWasRunning ) startScanCB();
    }
    static void toggleEchoCB( void * param = NULL ) {
      Out.serialEcho = !Out.serialEcho;
      setPrefs();
      Serial.printf("Out.serialEcho = %s\n", Out.serialEcho ? "true" : "false" );
    }
    static void rmFileCB( void * param = NULL ) {
      xTaskCreatePinnedToCore(rmFileTask, "rmFileTask", 5000, param, 2, NULL, 1); /* last = Task Core */
    }
    static void rmFileTask( void * param = NULL ) {
      // YOLO style
      isQuerying = true;
      if ( param != NULL ) {
        if ( BLE_FS.remove( (const char*)param ) ) {
          Serial.printf("File %s deleted\n", param);
        } else {
          Serial.printf("File %s could not be deleted\n", param);
        }
      } else {
        Serial.println("Nothing to delete");
      }
      isQuerying = false;
      vTaskDelete( NULL );
    }
    static void screenShowCB( void * param = NULL ) {
      xTaskCreate(screenShowTask, "screenShowTask", 16000, param, 2, NULL);
    }

    static void screenShowTask( void * param = NULL ) {
      UI.screenShow( param );
      vTaskDelete(NULL);
    }

    static void screenShotCB( void * param = NULL ) {
      xTaskCreate(screenShotTask, "screenShotTask", 16000, NULL, 2, NULL);
    }
    static void screenShotTask( void * param = NULL ) {
      if( !UI.ScreenShotLoaded ) {
        M5.ScreenShot.init( &tft, BLE_FS );
        M5.ScreenShot.begin();
        UI.ScreenShotLoaded = true;
      }
      UI.screenShot();
      vTaskDelete(NULL);
    }
    static void listDirCB( void * param = NULL ) {
      xTaskCreatePinnedToCore(listDirTask, "listDirTask", 5000, param, 2, NULL, 1); /* last = Task Core */
    }
    static void listDirTask( void * param = NULL ) {
      isQuerying = true;
      if( param != NULL ) {
        if(! BLE_FS.exists( (const char*)param ) ) {
          Serial.printf("Directory %s does not exist");
        } else {
          listDir(BLE_FS, (const char*)param, 0, DB.BLEMacsDbFSPath);
        }
      } else {
        listDir(BLE_FS, "/", 0, DB.BLEMacsDbFSPath);
      }
      isQuerying = false;
      vTaskDelete( NULL );
    }
    static void toggleCB( void * param = NULL ) {
      bool setbool = true;
      if ( param != NULL ) {
        //
      } else {
        setbool = false;
        Serial.println("\nCurrent property values:");
      }
      for ( uint16_t i = 0; i < Tsize; i++ ) {
        if ( setbool ) {
          if ( strcmp( TogglableProps[i].name, (const char*)param ) == 0 ) {
            TogglableProps[i].flag = !TogglableProps[i].flag;
            Serial.printf("Toggled flag %s to %s\n", TogglableProps[i].name, TogglableProps[i].flag ? "true" : "false");
          }
        } else {
          Serial.printf("  %24s : [%s]\n", TogglableProps[i].name, TogglableProps[i].flag ? "true" : "false");
        }
      }
    }

    static void nullCB( void * param = NULL ) {
      if ( param != NULL ) {
        Serial.printf("nullCB param: %s\n", param);
      }
      // zilch, niente, nada, que dalle, nothing
    }
    static void startSerialTask() {
      serialBuffer = (char*)calloc( SERIAL_BUFFER_SIZE, sizeof(char) );
      tempBuffer   = (char*)calloc( SERIAL_BUFFER_SIZE, sizeof(char) );
      xTaskCreatePinnedToCore(serialTask, "serialTask", 2048 + SERIAL_BUFFER_SIZE, NULL, 0, NULL, 1); /* last = Task Core */
    }

    static void serialTask( void * parameter ) {
      CommandTpl Commands[] = {
        { "help",          nullCB,                 "Print this list" },
        { "start",         startScanCB,            "Start/resume scan" },
        { "stop",          stopScanCB,             "Stop scan" },
        { "toggleFilter",  toggleFilterCB,         "Toggle vendor filter on the TFT (persistent)" },
        { "toggleEcho",    toggleEchoCB,           "Toggle BLECards in the Serial Console (persistent)" },
        { "dump",          startDumpCB,            "Dump returning BLE devices to the display and updates DB" },
        { "setBrightness", setBrightnessCB,        "Set brightness to [value] (0-255)" },
        { "ls",            listDirCB,              "Show [dir] Content on the SD" },
        { "rm",            rmFileCB,               "Delete [file] from the SD" },
        { "restart",       restartCB,              "Restart BLECollector ('restart now' to skip replication)" },
        #if HAS_EXTERNAL_RTC
          { "bleclock",      setTimeServerOn,        "Broadcast time to another BLE Device (implicit)" },
          { "bletime",       setTimeClientOn,        "Get time from another BLE Device (explicit)" },
        #else
          { "bleclock",      setTimeServerOn,        "Broadcast time to another BLE Device (explicit)" },
          { "bletime",       setTimeClientOn,        "Get time from another BLE Device (implicit)" },
        #endif
        { "blereceive",    setFileSharingServerOn, "Update .db files from another BLE app"},
        { "blesend",       setFileSharingClientOn, "Share .db files with anothe BLE app" },
        { "screenshot",    screenShotCB,           "Make a screenshot and save it on the SD" },
        { "screenshow",    screenShowCB,           "Show screenshot" },
        { "toggle",        toggleCB,               "toggle a bool value" },
        #if HAS_GPS
          { "gpstime",       setGPSTime,     "sync time from GPS" },
        #endif
        { "resetDB",       resetCB,        "Hard Reset DB + forced restart" },
        { "pruneDB",       pruneCB,        "Soft Reset DB without restarting (hopefully)" },
        #ifdef WITH_WIFI
          { "stopBLE",       stopBLECB,      "Stop BLE and start WiFi (experimental)" },
          { "setWiFiSSID",   setWiFiSSID,    "Set WiFi SSID" },
          { "setWiFiPASS",   setWiFiPASS,    "Set WiFi Password" },
        #endif
      };

      SerialCommands = Commands;
      Csize = (sizeof(Commands) / sizeof(Commands[0]));

      ToggleTpl ToggleProps[] = {
        { "Out.serialEcho",      Out.serialEcho },
        { "DB.needsReset",       DB.needsReset },
        { "DBneedsReplication",  DBneedsReplication },
        { "DB.needsPruning",     DB.needsPruning },
        { "TimeIsSet",           TimeIsSet },
        { "foundTimeServer",     foundTimeServer },
        { "RTCisRunning",        RTCisRunning },
        { "ForceBleTime",        ForceBleTime },
        { "DayChangeTrigger",    DayChangeTrigger },
        { "HourChangeTrigger",   HourChangeTrigger },
        { "fileSharingEnabled",  fileSharingEnabled },
        { "timeServerIsRunning", timeServerIsRunning }
      };
      TogglableProps = ToggleProps;
      Tsize = (sizeof(ToggleProps) / sizeof(ToggleProps[0]));

      if (resetReason != 12) { // HW Reset
        runCommand( (char*)"help" );
        //runCommand( (char*)"toggle" );
        //runCommand( (char*)"ls" );
      }
      if( TimeIsSet ) {
        // auto share time if available
        runCommand( (char*)"bleclock" );
      }

      #if HAS_GPS
        GPSInit();
      #endif

      static byte idx = 0;
      char lf = '\n';
      char cr = '\r';
      char c;
      unsigned long lastHidCheck = millis();
      while ( 1 ) {
        while (Serial.available() > 0) {
          c = Serial.read();
          if (c != cr && c != lf) {
            serialBuffer[idx] = c;
            idx++;
            if (idx >= SERIAL_BUFFER_SIZE) {
              idx = SERIAL_BUFFER_SIZE - 1;
            }
          } else {
            serialBuffer[idx] = '\0'; // null terminate
            memcpy( tempBuffer, serialBuffer, idx + 1 );
            runCommand( tempBuffer );
            idx = 0;
          }
          vTaskDelay(1);
        }
        #if HAS_GPS
          GPSRead();
        #endif
        if( hasHID() ) {
          if( lastHidCheck + 150 < millis() ) {
            M5.update();
            if( M5.BtnA.wasPressed() ) {
              UI.brightness -= UI.brightnessIncrement;
              setBrightnessCB();
            }
            if( M5.BtnB.wasPressed() ) {
              UI.brightness += UI.brightnessIncrement;
              setBrightnessCB();
            }
            if( M5.BtnC.wasPressed() ) {
              toggleFilterCB();
            }
            lastHidCheck = millis();
          }
        }
        vTaskDelay(1);
      }
    }


    static void runCommand( char* command ) {
      if ( isEmpty( command ) ) return;
      if ( strcmp( command, "help" ) == 0 ) {
        Serial.println("\nAvailable Commands:\n");
        for ( uint16_t i = 0; i < Csize; i++ ) {
          Serial.printf("  %02d) %16s : %s\n", i + 1, SerialCommands[i].command, SerialCommands[i].description);
        }
        Serial.println();
      } else {
        char *token;
        char delim[2];
        char *args;
        bool has_args = false;
        strncpy(delim, " ", 2); // strtok_r needs a null-terminated string

        if ( strstr(command, delim) ) {
          // turn command into token/arg
          token = strtok_r(command, delim, &args); // Search for command at start of buffer
          if ( token != NULL ) {
            has_args = true;
            //Serial.printf("[%s] Found arg for token '%s' : %s\n", command, token, args);
          }
        }
        for ( uint16_t i = 0; i < Csize; i++ ) {
          if ( strcmp( SerialCommands[i].command, command ) == 0 ) {
            if ( has_args ) {
              Serial.printf( "Running '%s %s' command\n", token, args );
              SerialCommands[i].cb.function( args );
            } else {
              Serial.printf( "Running '%s' command\n", SerialCommands[i].command );
              SerialCommands[i].cb.function( NULL );
            }
            //sprintf(command, "%s", "");
            return;
          }
        }
        Serial.printf( "Command '%s' not found\n", command );
      }
    }


    static void scanTask( void * parameter ) {
      UI.update(); // run after-scan display stuff
      DB.maintain();
      scanTaskRunning = true;
      scanTaskStopped = false;
      if ( FoundDeviceCallback == NULL ) {
        FoundDeviceCallback = new FoundDeviceCallbacks(); // collect/store BLE data
      }
      pBLEScan = BLEDevice::getScan(); //create new scan
      pBLEScan->setAdvertisedDeviceCallbacks( FoundDeviceCallback );
      pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
      pBLEScan->setInterval(0x50); // 0x50
      pBLEScan->setWindow(0x30); // 0x30
      byte onAfterScanStep = 0;
      while ( scanTaskRunning ) {
        if ( onAfterScanSteps( onAfterScanStep, scan_cursor ) ) continue;
        dumpStats("BeforeScan::");
        onBeforeScan();
        pBLEScan->start(SCAN_DURATION);
        onAfterScan();
        UI.update(); // run after-scan display stuff
        //DB.maintain();
        dumpStats("AfterScan:::");
        scan_rounds++;
      }
      scanTaskStopped = true;
      delete FoundDeviceCallback; FoundDeviceCallback = NULL;
      vTaskDelete( NULL );
    }


    static bool onAfterScanSteps( byte &onAfterScanStep, uint16_t &scan_cursor ) {
      switch ( onAfterScanStep ) {
        case POPULATE: // 0
          onScanPopulate( scan_cursor ); // OUI / vendorname / isanonymous
          onAfterScanStep++;
          return true;
          break;
        case IFEXISTS: // 1
          onScanIfExists( scan_cursor ); // exists + hits
          onAfterScanStep++;
          return true;
          break;
        case RENDER: // 2
          onScanRender( scan_cursor ); // ui work
          onAfterScanStep++;
          return true;
          break;
        case PROPAGATE: // 3
          onAfterScanStep = 0;
          if ( onScanPropagate( scan_cursor ) ) { // copy to DB / cache
            scan_cursor++;
            return true;
          }
          break;
        default:
          log_w("Exit flat loop on afterScanStep value : %d", onAfterScanStep);
          onAfterScanStep = 0;
          break;
      }
      return false;
    }


    static bool onScanPopulate( uint16_t _scan_cursor ) {
      if ( onScanPopulated ) {
        log_v("%s", " onScanPopulated = true ");
        return false;
      }
      if ( _scan_cursor >= devicesCount) {
        onScanPopulated = true;
        log_d("%s", "done all");
        return false;
      }
      if ( isEmpty( BLEDevScanCache[_scan_cursor]->address ) ) {
        log_w("empty addess");
        return true; // end of cache
      }
      populate( BLEDevScanCache[_scan_cursor] );
      return true;
    }


    static bool onScanIfExists( int _scan_cursor ) {
      if ( onScanPostPopulated ) {
        log_v("onScanPostPopulated = true");
        return false;
      }
      if ( _scan_cursor >= devicesCount) {
        log_d("done all");
        onScanPostPopulated = true;
        return false;
      }
      int deviceIndexIfExists = -1;
      deviceIndexIfExists = getDeviceCacheIndex( BLEDevScanCache[_scan_cursor]->address );
      if ( deviceIndexIfExists > -1 ) {
        inCacheCount++;
        BLEDevRAMCache[deviceIndexIfExists]->hits++;
        if ( TimeIsSet ) {
          if ( BLEDevRAMCache[deviceIndexIfExists]->created_at.year() <= 1970 ) {
            BLEDevRAMCache[deviceIndexIfExists]->created_at = nowDateTime;
          }
          BLEDevRAMCache[deviceIndexIfExists]->updated_at = nowDateTime;
        }
        BLEDevHelper.mergeItems( BLEDevScanCache[_scan_cursor], BLEDevRAMCache[deviceIndexIfExists] ); // merge scan data into existing psram cache
        BLEDevHelper.copyItem( BLEDevRAMCache[deviceIndexIfExists], BLEDevScanCache[_scan_cursor] ); // copy back merged data for rendering
        log_i( "Device %d / %s exists in cache, increased hits to %d", _scan_cursor, BLEDevScanCache[_scan_cursor]->address, BLEDevScanCache[_scan_cursor]->hits );
      } else {
        if ( BLEDevScanCache[_scan_cursor]->is_anonymous ) {
          // won't land in DB (won't be checked either) but will land in cache
          uint16_t nextCacheIndex = BLEDevHelper.getNextCacheIndex( BLEDevRAMCache, BLEDevCacheIndex );
          BLEDevHelper.reset( BLEDevRAMCache[nextCacheIndex] );
          BLEDevScanCache[_scan_cursor]->hits++;
          BLEDevHelper.copyItem( BLEDevScanCache[_scan_cursor], BLEDevRAMCache[nextCacheIndex] );
          log_i( "Device %d / %s is anonymous, won't be inserted", _scan_cursor, BLEDevScanCache[_scan_cursor]->address, BLEDevScanCache[_scan_cursor]->hits );
        } else {
          deviceIndexIfExists = DB.deviceExists( BLEDevScanCache[_scan_cursor]->address ); // will load returning devices from DB if necessary
          if (deviceIndexIfExists > -1) {
            uint16_t nextCacheIndex = BLEDevHelper.getNextCacheIndex( BLEDevRAMCache, BLEDevCacheIndex );
            BLEDevHelper.reset( BLEDevRAMCache[nextCacheIndex] );
            BLEDevDBCache->hits++;
            if ( TimeIsSet ) {
              if ( BLEDevDBCache->created_at.year() <= 1970 ) {
                BLEDevDBCache->created_at = nowDateTime;
              }
              BLEDevDBCache->updated_at = nowDateTime;
            }
            BLEDevHelper.mergeItems( BLEDevScanCache[_scan_cursor], BLEDevDBCache ); // merge scan data into BLEDevDBCache
            BLEDevHelper.copyItem( BLEDevDBCache, BLEDevRAMCache[nextCacheIndex] ); // copy merged data to assigned psram cache
            BLEDevHelper.copyItem( BLEDevDBCache, BLEDevScanCache[_scan_cursor] ); // copy back merged data for rendering

            log_i( "Device %d / %s is already in DB, increased hits to %d", _scan_cursor, BLEDevScanCache[_scan_cursor]->address, BLEDevScanCache[_scan_cursor]->hits );
          } else {
            // will be inserted after rendering
            BLEDevScanCache[_scan_cursor]->in_db = false;
            log_i( "Device %d / %s is not in DB", _scan_cursor, BLEDevScanCache[_scan_cursor]->address );
          }
        }
      }
      return true;
    }


    static bool onScanRender( uint16_t _scan_cursor ) {
      if ( onScanRendered ) {
        log_v("onScanRendered = true");
        return false;
      }
      if ( _scan_cursor >= devicesCount) {
        log_d("done all");
        onScanRendered = true;
        return false;
      }
      UI.BLECardTheme.setTheme( IN_CACHE_ANON );
      BLEDevTmp = BLEDevScanCache[_scan_cursor];
      UI.printBLECard( BLEDevTmp ); // render
      delay(1);
      sprintf( processMessage, processTemplateLong, "Rendered ", _scan_cursor + 1, " / ", devicesCount );
      UI.headerStats( processMessage );
      delay(1);
      UI.cacheStats();
      delay(1);
      UI.footerStats();
      delay(1);
      return true;
    }


    static bool onScanPropagate( uint16_t &_scan_cursor ) {
      if ( onScanPropagated ) {
        log_v("onScanPropagated = true");
        return false;
      }
      if ( _scan_cursor >= devicesCount) {
        log_d("done all");
        onScanPropagated = true;
        _scan_cursor = 0;
        return false;
      }
      //BLEDevScanCacheIndex = _scan_cursor;
      if ( isEmpty( BLEDevScanCache[_scan_cursor]->address ) ) {
        return true;
      }
      if ( BLEDevScanCache[_scan_cursor]->is_anonymous || BLEDevScanCache[_scan_cursor]->in_db ) { // don't DB-insert anon or duplicates
        sprintf( processMessage, processTemplateLong, "Released ", _scan_cursor + 1, " / ", devicesCount );
        if ( BLEDevScanCache[_scan_cursor]->is_anonymous ) AnonymousCacheHit++;
      } else {
        if ( DB.insertBTDevice( BLEDevScanCache[_scan_cursor] ) == DBUtils::INSERTION_SUCCESS ) {
          sprintf( processMessage, processTemplateLong, "Saved ", _scan_cursor + 1, " / ", devicesCount );
          log_d( "Device %d successfully inserted in DB", _scan_cursor );
          entries++;
        } else {
          log_e( "  [!!! BD INSERT FAIL !!!] Device %d could not be inserted", _scan_cursor );
          sprintf( processMessage, processTemplateLong, "Failed ", _scan_cursor + 1, " / ", devicesCount );
        }
      }
      BLEDevHelper.reset( BLEDevScanCache[_scan_cursor] ); // discard
      UI.headerStats( processMessage );
      return true;
    }


    static void onBeforeScan() {
      DB.maintain();
      UI.headerStats("Scan in progress");
      UI.startBlink();
      processedDevicesCount = 0;
      devicesCount = 0;
      scan_cursor = 0;
      onScanProcessed = false;
      onScanDone = false;
      onScanPopulated = false;
      onScanPropagated = false;
      onScanPostPopulated = false;
      onScanRendered = false;
      foundTimeServer = false;
      foundFileServer = false;
    }

    static void onAfterScan() {

      UI.stopBlink();

      if ( fileSharingEnabled ) { // found a peer to share with ?
        if( !fileSharingClientStarted ) { // fire file sharing client task
          if( fileServerBLEAddress != "" ) {
            UI.headerStats("File Sharing ...");
            log_w("Launching FileSharingClient Task");
            xTaskCreatePinnedToCore( startFileSharingClient, "startFileSharingClient", 2048, NULL, 5, NULL, 1 );
            while( scanTaskRunning ) {
              vTaskDelay( 10 );
            }
            return;
          }
        }
      }

      if ( foundTimeServer && (!TimeIsSet || ForceBleTime) ) {
        if( ! timeClientisStarted ) {
          if( timeServerBLEAddress != "" ) {
            UI.headerStats("BLE Time sync ...");
            log_w("Launching BLE TimeClient Task");
            xTaskCreatePinnedToCore(startTimeClient, "startTimeClient", 2048, NULL, 0, NULL, 0); /* last = Task Core */
            while( scanTaskRunning ) {
              vTaskDelay( 10 );
            }
            return;
          }
        }
      }

      UI.headerStats("Showing results ...");
      devicesCount = processedDevicesCount;
      BLEDevice::getScan()->clearResults();
      if ( devicesCount < MAX_DEVICES_PER_SCAN ) {
        if ( SCAN_DURATION + 1 < MAX_SCAN_DURATION ) {
          SCAN_DURATION++;
        }
      } else if ( devicesCount > MAX_DEVICES_PER_SCAN ) {
        if ( SCAN_DURATION - 1 >= MIN_SCAN_DURATION ) {
          SCAN_DURATION--;
        }
        log_w("Cache overflow (%d results vs %d slots), truncating results...", devicesCount, MAX_DEVICES_PER_SCAN);
        devicesCount = MAX_DEVICES_PER_SCAN;
      } else {
        // same amount
      }
      sessDevicesCount += devicesCount;
      notInCacheCount = 0;
      inCacheCount = 0;
      onScanDone = true;
      scan_cursor = 0;
    }


    static int getDeviceCacheIndex(const char* address) {
      if ( isEmpty( address ) )  return -1;
      for (int i = 0; i < BLEDEVCACHE_SIZE; i++) {
        if ( strcmp(address, BLEDevRAMCache[i]->address ) == 0  ) {
          BLEDevCacheHit++;
          log_v("[CACHE HIT] BLEDevCache ID #%s has %d cache hits", address, BLEDevRAMCache[i]->hits);
          return i;
        }
        delay(1);
      }
      return -1;
    }

    // used for serial debugging
    static void dumpStats(const char* prefixStr) {
      if (lastheap > freeheap) {
        // heap decreased
        sprintf(heapsign, "%s", "↘");
      } else if (lastheap < freeheap) {
        // heap increased
        sprintf(heapsign, "%s", "↗");
      } else {
        // heap unchanged
        sprintf(heapsign, "%s", "⇉");
      }
      if (lastscanduration > SCAN_DURATION) {
        sprintf(scantimesign, "%s", "↘");
      } else if (lastscanduration < SCAN_DURATION) {
        sprintf(scantimesign, "%s", "↗");
      } else {
        sprintf(scantimesign, "%s", "⇉");
      }

      lastheap = freeheap;
      lastscanduration = SCAN_DURATION;

      log_i("%s[Scan#%02d][%s][Duration%s%d][Processed:%d of %d][Heap%s%d / %d] [Cache hits][Screen:%d][BLEDevCards:%d][Anonymous:%d][Oui:%d][Vendor:%d]\n",
        prefixStr,
        scan_rounds,
        hhmmssString,
        scantimesign,
        lastscanduration,
        processedDevicesCount,
        devicesCount,
        heapsign,
        lastheap,
        freepsheap,
        SelfCacheHit,
        BLEDevCacheHit,
        AnonymousCacheHit,
        OuiCacheHit,
        VendorCacheHit
       );
    }

  private:

    static void getPrefs() {
      preferences.begin("BLEPrefs", true);
      Out.serialEcho   = preferences.getBool("serialEcho", true);
      UI.filterVendors = preferences.getBool("filterVendors", false);
      UI.brightness    = preferences.getUChar("brightness", BASE_BRIGHTNESS);
      log_w("Defrosted brightness: %d", UI.brightness );
      preferences.end();
    }
    static void setPrefs() {
      preferences.begin("BLEPrefs", false);
      preferences.putBool("serialEcho", Out.serialEcho);
      preferences.putBool("filterVendors", UI.filterVendors );
      preferences.putUChar("brightness", UI.brightness );
      preferences.end();
    }

    // completes unpopulated fields of a given entry by performing DB oui/vendor lookups
    static void populate( BlueToothDevice *CacheItem ) {
      if ( strcmp( CacheItem->ouiname, "[unpopulated]" ) == 0 ) {
        log_d("  [populating OUI for %s]", CacheItem->address);
        DB.getOUI( CacheItem->address, CacheItem->ouiname );
      }
      if ( strcmp( CacheItem->manufname, "[unpopulated]" ) == 0 ) {
        if ( CacheItem->manufid != -1 ) {
          log_d("  [populating Vendor for :%d]", CacheItem->manufid );
          DB.getVendor( CacheItem->manufid, CacheItem->manufname );
        } else {
          BLEDevHelper.set( CacheItem, "manufname", '\0');
        }
      }
      CacheItem->is_anonymous = BLEDevHelper.isAnonymous( CacheItem );
      log_d("[populated :%s]", CacheItem->address);
    }

};




BLEScanUtils BLECollector;

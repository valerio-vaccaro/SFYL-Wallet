/*
                   _____ ________  ____
                  / ___// ____/\ \/ / /
                  \__ \/ /_     \  / /
                 ___/ / __/     / / /___
                /____/_/       /_/_____/ Wallet
                  Valerio Vaccaro - 2021

Upload spifs content with the command: pio run --target uploadfs
*/
#include <Arduino.h>
#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "config.h"
#include <Bitcoin.h>
#include <PSBT.h>
#include <Hash.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>
#include <ESPmDNS.h>

// Disable Tcp Watchdog
#define CONFIG_ASYNC_TCP_USE_WDT 0

AsyncWebServer server(80);

String walletName = "default";
bool walletModified = false;
String network = "";
String mnemonic = "";
String passphrase = "";
String path = "";

String xpub = "";
String xpubcore = "";
String descriptorCore = "";
String descriptorCoreChange = "";

int first = 0;
int second = 0;
String address = "";

String unsignedpsbt = "";
String signedpsbt = "";

String message = "Buy Bitcoin!";
String signature = "";

String ssid = "";
String password = "";
boolean showAp = false;

extern const unsigned char logoIcon[134*28];
#define DEFAULT_BRIGHTNESS                          45
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
TTGOClass *twatch = nullptr;
GxEPD_Class *ePaper = nullptr;
PCF8563_Class *rtc = nullptr;
AXP20X_Class *power = nullptr;
Button2 *btn = nullptr;
uint32_t seupCount = 0;
bool pwIRQ = false;

void setupDisplay(){
    u8g2Fonts.begin(*ePaper);                   // connect u8g2 procedures to Adafruit GFX
    u8g2Fonts.setFontMode(1);                   // use u8g2 transparent mode (this is default)
    u8g2Fonts.setFontDirection(0);              // left to right (this is default)
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);  // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);  // apply Adafruit GFX color
    u8g2Fonts.setFont(u8g2_font_inr38_mn);      // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
}

void mainPage(bool fullScreen){
    static uint8_t hh = 0, mm = 0;
    static uint8_t lastDay = 0;
    char buff[64] = "00:00";
    int battPerc, battPercView;
    //int battLoad;

    RTC_Date d = rtc->getDateTime();
    if (mm == d.minute && !fullScreen) {
        return ; // nothing changed
    }

    mm = d.minute;
    hh = d.hour;
    if (lastDay != d.day) {
        lastDay = d.day;
        fullScreen = true;  // repaint all
    }

    snprintf(buff, sizeof(buff), "%02d:%02d", hh, mm);

    if (fullScreen) {
        // Print logo
        ePaper->drawBitmap(5, 5, logoIcon, 134, 28, GxEPD_BLACK);

        // Print battery icon
        /*battLoad=0;
        if (twatch->power->isVBUSPlug())               // check if battery is charging
          battLoad=1;*/
        battPerc=twatch->power->getBattPercentage();   // get the status of the battery
        battPercView=((battPerc+10)/20);               // there are 10 different states to display the percentage of battery charge

        u8g2Fonts.setFont(u8g2_font_battery19_tn);
        u8g2Fonts.setCursor(175, 25);                // start writing at this position
        u8g2Fonts.setFontDirection(3);               // left to right (this is default)
        u8g2Fonts.print(battPercView);
        u8g2Fonts.setFontDirection(0);               // left to right (this is default)

        // draw line
        ePaper->drawFastHLine(10, 40, ePaper->width() - 20, GxEPD_BLACK);

        // draw title
        u8g2Fonts.setFont(u8g2_font_helvB18_tf);
        int offset_x = (ePaper->width() - u8g2Fonts.getUTF8Width("SFYL Wallet")) / 2;
        u8g2Fonts.setCursor(offset_x, 70);
        u8g2Fonts.print("SFYL Wallet");

        // draw clock
        u8g2Fonts.setFont(u8g2_font_7Segments_26x42_mn);
        u8g2Fonts.setCursor(25, 120);
        u8g2Fonts.print(buff);

        // draw line
        ePaper->drawFastHLine(10, 130, ePaper->width() - 20, GxEPD_BLACK);

        if (showAp){
          // draw strings
          ePaper->fillRect(20, 140, ePaper->width() - 40, 169, GxEPD_WHITE);
          u8g2Fonts.setFont(u8g2_font_helvB10_tf); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
          u8g2Fonts.setCursor(20, 150);
          u8g2Fonts.print("SSID: "+ssid);
          u8g2Fonts.setCursor(20, 170);
          u8g2Fonts.print("Password: "+password);
        } else {
          // draw strings
          ePaper->fillRect(20, 140, ePaper->width() - 40, 169, GxEPD_WHITE);
          u8g2Fonts.setFont(u8g2_font_helvB10_tf); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
          u8g2Fonts.setCursor(20, 150);
          u8g2Fonts.print("SSID: ********");
          u8g2Fonts.setCursor(20, 170);
          u8g2Fonts.print("Password: ********");
        }

        // draw line
        ePaper->drawFastHLine(10, 180, ePaper->width() - 20, GxEPD_BLACK);

        // draw motto
        String motto = "PER ASPERA AD ASTRA";
        offset_x = (ePaper->width() - u8g2Fonts.getUTF8Width(motto.c_str())) / 2;
        u8g2Fonts.setCursor(offset_x, 200);
        u8g2Fonts.print(motto);

        ePaper->update();
    } else {
        // update clock
        ePaper->fillRect(20, 70, ePaper->width() - 40, 49, GxEPD_WHITE);
        ePaper->fillScreen(GxEPD_WHITE);
        ePaper->setTextColor(GxEPD_BLACK);
        u8g2Fonts.setFont(u8g2_font_7Segments_26x42_mn);
        u8g2Fonts.setCursor(25, 120);
        u8g2Fonts.print(buff);

        ePaper->updateWindow(20, 70, ePaper->width() - 40, 50,  false);
    }
}

String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool loadWallet(String filename, String password) {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }
  String fileContent;
  while (file.available()) {
    fileContent += char(file.read());
  }
  file.close();
  network = getValue(fileContent, ',', 0);
  mnemonic =  getValue(fileContent, ',', 1);
  passphrase = getValue(fileContent, ',', 2);
  path =  getValue(fileContent, ',', 3);
  return true;
}

bool saveWallet(String filename, String password){
  if (filename == "/default.wal")
    return false;
  // create a spiffs file
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
      Serial.println("There was an error opening the file for writing");
      return false;
  }
  String s = network+","+mnemonic+","+passphrase+","+path;
  if (!file.print(s.c_str()))
      Serial.println("File write failed");
  file.close();
  return true;
}

bool deleteWallet(String filename){
  if (filename == "/default.wal")
    return false;
  return SPIFFS.remove(filename);
}

String listWallet(bool options){
  String files;
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
    if (getValue(file.name(), '.', 1) == "wal"){
      String fname = getValue(file.name(), '.', 0);
      fname.remove(0, 1);
      if (options) {
        files += "<option value=\"" + fname + "\">" + fname + "</option>";
      } else {
        files += fname + " ";
      }
    }
    file = root.openNextFile();
  }
  return files;
}

String processor(const String& var) {
  if (var == "MAC")          {
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    return String(macAddr[0], HEX)+String(macAddr[1], HEX)+String(macAddr[2], HEX)+String(macAddr[3], HEX)+String(macAddr[4], HEX)+String(macAddr[5], HEX);
  }
  if (var == "BATTERY")             return String(twatch->power->getBattPercentage());
  if (var == "HEAP")                return String(ESP.getFreeHeap());
  if (var == "CHIPID")              {
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    uint32_t chipid = macAddressTrunc >> 40;
    return String(chipid, HEX);
  }
  if (var == "WALLETNAME")           return walletName;
  if (var == "WALLETMODIFIED")       return (walletModified) ? "Wallet is differrent from saved copy!" : "";
  if (var == "NETWORK")              return network;
  if (var == "MNEMONIC")             return mnemonic;
  if (var == "PATH")                 return path;
  if (var == "XPUB")                 return xpub;
  if (var == "XPUBCORE")             return xpubcore;
  if (var == "DESCRIPTORCORE")       return descriptorCore;
  if (var == "DESCRIPTORCORECHANGE") return descriptorCoreChange;
  if (var == "FIRST")                return String(first);
  if (var == "SECOND")               return String(second);
  if (var == "ADDRESS")              return address;
  if (var == "UNSIGNEDPSBT")         return unsignedpsbt;
  if (var == "SIGNEDPSBT")           return signedpsbt;
  if (var == "MESSAGE")              return message;
  if (var == "SIGNATURE")            return signature;
  if (var == "FILES")                return listWallet(true);
  if (var == "WALLETS")              return listWallet(false);
  return String();
}



void setup() {
    // Disable Watchdog
    disableCore0WDT();
    disableCore1WDT();
    disableLoopWDT();

    // Serial for debug only
    Serial.begin(115200);
    delay(100);

    // Load spiffs
    if(!SPIFFS.begin()){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
    }

    // Load default wallet
    loadWallet("/default.wal", "");

    // Get watch object
    twatch = TTGOClass::getWatch();
    twatch->begin();

    // Turn on the backlight
    twatch->openBL();
    rtc = twatch->rtc;
    power = twatch->power;
    btn = twatch->button;
    ePaper = twatch->ePaper;

    // Use compile time as RTC input time
    rtc->check();

    // Turn on power management button interrupt
    power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);

    // Clear power interruption
    power->clearIRQ();

    // Set MPU6050 to sleep
    twatch->mpu->setSleepEnabled(true);

    // Set Pin to interrupt
    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        pwIRQ = true;
    }, FALLING);

    btn->setPressedHandler([]() {
        delay(2000);
        esp_sleep_enable_ext1_wakeup(GPIO_SEL_36, ESP_EXT1_WAKEUP_ALL_LOW);
        esp_deep_sleep_start();
    });

    // Adjust the backlight to reduce current consumption
    twatch->setBrightness(DEFAULT_BRIGHTNESS);

    // Initialize the ink screen
    setupDisplay();

    // AP ssid and password
    ssid = "SFYL_"+String(esp_random(), HEX);
    password = String(esp_random(), HEX);

    // Initialize the interface
    mainPage(true);

    // Reduce CPU frequency
    //setCpuFrequencyMhz(80);

    // WIFI
    ssid = "esp32";
    password = "12345678";
    WiFi.softAP(ssid.c_str(), password.c_str(), 1, 0, 1); // Max 1 connection
    delay(100);

    // mDNS sfyl.local
    if (!MDNS.begin("sfyl")) {
        Serial.println("Error setting up MDNS responder!");
        return;
    }

    // Webserver
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });

    server.on("/xpub", HTTP_GET, [](AsyncWebServerRequest *request){
      // fix pathcore
      String pathcore = path;
      pathcore.replace("m","");
      pathcore.replace("'","h");

      // derive main xprv
      HDPrivateKey root(mnemonic, passphrase);
      // derive account
      HDPrivateKey account = root.derive(path);
      HDPublicKey xpub_key = account.xpub();
      xpub = "["+root.fingerprint()+pathcore+"]"+xpub_key.toString();

      HDPrivateKey account_core = root.derive(path);
      // avoid funny name for xpub
      account_core.type = UNKNOWN_TYPE;
      HDPublicKey xpub_key_core = account_core.xpub();
      xpubcore = xpub_key_core.toString();

      descriptorCore = "wpkh([";
      descriptorCore += root.fingerprint();
      descriptorCore += pathcore + "]";
      descriptorCore += xpubcore;
      descriptorCore += "/0/*)";
      descriptorCore += String("#")+descriptorChecksum(descriptorCore);

      descriptorCoreChange = "wpkh([";
      descriptorCoreChange += root.fingerprint();
      descriptorCoreChange += pathcore + "]";
      descriptorCoreChange += xpubcore;
      descriptorCoreChange += "/1/*)";
      descriptorCoreChange += String("#")+descriptorChecksum(descriptorCoreChange);

      // derive first address
      HDPublicKey pub;
      pub = xpub_key.child(0).child(0);
      address = pub.segwitAddress(&Testnet);

      request->send(SPIFFS, "/xpub.html", "text/html", false, processor);
    });

    server.on("/address", HTTP_GET, [](AsyncWebServerRequest *request){
      String command_arg = "";
      if(request->hasArg("command"))
        command_arg = request->arg("command");
      if (command_arg == "getaddress"){
        first = request->arg("first").toInt();
        second = request->arg("second").toInt();
      } else {
        first = 0;
        second = 0;
      }
      HDPrivateKey root(mnemonic, passphrase);
      HDPrivateKey account = root.derive(path);
      account.type = UNKNOWN_TYPE;
      HDPublicKey xpub = account.xpub();
      HDPublicKey pub = xpub.child(first).child(second);
      address = pub.segwitAddress(&Testnet);
      request->send(SPIFFS, "/address.html", "text/html", false, processor);
    });

    server.on("/sign", HTTP_GET, [](AsyncWebServerRequest *request){
      String command_arg = "";
      if(request->hasArg("command"))
        command_arg = request->arg("command");
      if (command_arg == "psbt"){
        HDPrivateKey root(mnemonic, passphrase);
        HDPrivateKey account = root.derive(path);
        unsignedpsbt = request->arg("unsignedpsbt");
        Serial.println(unsignedpsbt);
        PSBT psbt;
        psbt.parseBase64(unsignedpsbt);

        // TODO decode psbt
        // TODO shows decoded psbt in the eink
        // TODO validate 2fa

        psbt.sign(root);
        signedpsbt = psbt.toBase64();

      } else if (command_arg == "message"){
        HDPrivateKey root(mnemonic, passphrase);
        HDPrivateKey account = root.derive(path);
        HDPrivateKey priv = account.child(0).child(0);
        address = priv.segwitAddress();

        message = request->arg("message");
        uint8_t hash[32];
        sha256(message.c_str(), strlen(message.c_str()), hash);
        Signature sig = priv.sign(hash);
        signature = String(sig);
      }

      request->send(SPIFFS, "/sign.html", "text/html", false, processor);
    });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
      String command_arg = "";
      if(request->hasArg("command"))
        command_arg = request->arg("command");
      if (command_arg == "settings"){
        // todo: implement some checks
        network = request->arg("network");
        mnemonic = request->arg("mnemonic");
        passphrase = request->arg("passphrase");
        path = request->arg("path");
        walletModified = true;
      }
      else if (command_arg == "load"){
        String filename = request->arg("filenameLoad");
        String password = request->arg("passwordLoad");
        if (loadWallet("/"+filename+".wal", password)) {
          walletModified = false;
          walletName = filename;
        }
      }
      else if (command_arg == "save"){
        String filename = request->arg("filenameSave");
        String password = request->arg("passwordSave");
        if (saveWallet("/"+filename+".wal", password)){
          walletModified = false;
          walletName = filename;
        }
      }
      else if (command_arg == "delete"){
        String filename = request->arg("filenameDelete");
        deleteWallet("/"+filename+".wal");
      }
      request->send(SPIFFS, "/settings.html", "text/html", false, processor);
    });

    // Static files
    server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/bootstrap.bundle.min.js", "text/javascript");
    });

    server.on("/jquery-3.6.0.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/jquery-3.6.0.min.js", "text/javascript");
    });

    server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/bootstrap.min.css", "text/css");
    });

    server.on("/qrcode.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/qrcode.min.js", "text/javascript");
    });

    server.on("/bitcoin.pdf", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/bitcoin.pdf", "application/pdf");
    });

    server.on("/btc.png", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/btc.png", "image/png");
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/favicon.ico", "image/x-icon");
    });

    server.begin();
}

uint32_t loopMillis = 0;

void loop()
{
    btn->loop();

    if (pwIRQ) {
        pwIRQ = false;
        // Get interrupt status
        power->readIRQ();
        if (power->isPEKShortPressIRQ()) {
            twatch->bl->isOn() ? twatch->bl->off() : twatch->bl->adjust(DEFAULT_BRIGHTNESS);
        }
        if (power->isPEKLongtPressIRQ()) {
            showAp =  !showAp;
            mainPage(true);
        }
        // After the interruption, you need to manually clear the interruption status
        power->clearIRQ();
    }

    if (millis() - loopMillis > 1000) {
        loopMillis = millis();
        // Partial refresh
        mainPage(false);
    }
}

// http://javl.github.io/image2cpp/ with inverted colors
const unsigned char logoIcon[134*28] = {
0x00, 0x1f, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xff, 0xf0, 0x00, 0x1f, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8,
0x00, 0x00, 0x03, 0xff, 0xfc, 0x00, 0x1e, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xf8, 0x00, 0x00, 0x07, 0xff, 0xfe, 0x00, 0x3e, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0xf8, 0x00, 0x00, 0x0f, 0xff, 0xff, 0x00, 0x3e, 0x00, 0x07, 0xe3, 0xe0, 0x00, 0x00, 0x00,
0x00, 0x01, 0xf8, 0x00, 0x00, 0x1f, 0xf9, 0x7f, 0x80, 0x3e, 0x00, 0x03, 0xc3, 0xe0, 0x00, 0x00,
0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x3f, 0xf9, 0x3f, 0xc0, 0x3c, 0x00, 0x00, 0x03, 0xe0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x80, 0x3f, 0xc0, 0x7c, 0x00, 0x00, 0x03, 0xc0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x80, 0x3f, 0xe0, 0x7c, 0x00, 0x00, 0x07,
0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x0f, 0xe0, 0x7f, 0xf0, 0x0f,
0x87, 0xff, 0x01, 0xff, 0x03, 0xfc, 0x01, 0xe0, 0x7f, 0xe0, 0x7f, 0xc3, 0x0f, 0xe0, 0x7f, 0xf8,
0x0f, 0x87, 0xff, 0x07, 0xff, 0x07, 0xff, 0x03, 0xe0, 0xff, 0xf0, 0xff, 0xc3, 0x8f, 0xe0, 0xff,
0xfc, 0x0f, 0x87, 0xfe, 0x0f, 0xfe, 0x0f, 0xff, 0x03, 0xe1, 0xff, 0xf8, 0xff, 0xc3, 0x0f, 0xf0,
0xff, 0xfe, 0x0f, 0x87, 0xfe, 0x1f, 0xfe, 0x1f, 0xff, 0x83, 0xe1, 0xff, 0xf8, 0xff, 0xc0, 0x0f,
0xf0, 0xf8, 0x3e, 0x1f, 0x0f, 0x80, 0x3f, 0x00, 0x3f, 0x0f, 0x83, 0xe1, 0xf0, 0xf8, 0xff, 0x80,
0x1f, 0xf0, 0xf0, 0x3e, 0x1f, 0x0f, 0x80, 0x7e, 0x00, 0x7e, 0x0f, 0x87, 0xc1, 0xe0, 0x7c, 0xff,
0x87, 0x0f, 0xf1, 0xf0, 0x3e, 0x1f, 0x0f, 0x80, 0x7c, 0x00, 0x7c, 0x07, 0x87, 0xc3, 0xe0, 0x78,
0xfe, 0x07, 0x0f, 0xe1, 0xf0, 0x3e, 0x1f, 0x0f, 0x00, 0x7c, 0x00, 0x78, 0x07, 0x87, 0xc3, 0xe0,
0xf8, 0x7e, 0x07, 0x0f, 0xe1, 0xf0, 0x3e, 0x1e, 0x1f, 0x00, 0xf8, 0x00, 0xf8, 0x07, 0x87, 0xc3,
0xe0, 0xf8, 0x7e, 0x00, 0x0f, 0xe1, 0xf0, 0x3e, 0x3e, 0x1f, 0x00, 0xf8, 0x00, 0xf8, 0x0f, 0x87,
0x83, 0xe0, 0xf8, 0x7f, 0x80, 0x1f, 0xe1, 0xe0, 0x3e, 0x3e, 0x1f, 0x00, 0xf8, 0x00, 0xf8, 0x0f,
0x8f, 0x83, 0xc0, 0xf8, 0x3f, 0x90, 0x7f, 0xc3, 0xe0, 0x7c, 0x3e, 0x1e, 0x00, 0xf8, 0x00, 0xf8,
0x0f, 0x8f, 0x87, 0xc0, 0xf0, 0x3f, 0x93, 0xff, 0xc3, 0xe0, 0x7c, 0x3c, 0x1e, 0x00, 0xfc, 0x00,
0xf8, 0x1f, 0x0f, 0x87, 0xc1, 0xf0, 0x1f, 0xf3, 0xff, 0x83, 0xe0, 0xf8, 0x7c, 0x3f, 0x00, 0x7c,
0x00, 0xf8, 0x3f, 0x0f, 0x07, 0xc1, 0xf0, 0x0f, 0xff, 0xff, 0x03, 0xe3, 0xf8, 0x7c, 0x1f, 0x88,
0x7e, 0x18, 0x7e, 0x7e, 0x1f, 0x07, 0x81, 0xf0, 0x07, 0xff, 0xfe, 0x07, 0xff, 0xf0, 0x7c, 0x1f,
0xf8, 0x7f, 0xf8, 0x7f, 0xfe, 0x1f, 0x0f, 0x81, 0xe0, 0x03, 0xff, 0xfc, 0x07, 0xff, 0xe0, 0x78,
0x1f, 0xf8, 0x3f, 0xf8, 0x3f, 0xf8, 0x1f, 0x0f, 0x83, 0xe0, 0x00, 0xff, 0xf0, 0x07, 0xff, 0x80,
0xf8, 0x0f, 0xf8, 0x1f, 0xf8, 0x1f, 0xf0, 0x1e, 0x0f, 0x83, 0xe0, 0x00, 0x1f, 0x80, 0x00, 0xfc,
0x00, 0x00, 0x03, 0xe0, 0x03, 0xe0, 0x07, 0xc0, 0x00, 0x00, 0x00, 0x00 };

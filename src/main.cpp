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
#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"

AsyncWebServer server(80);

String network = "testnet";
String mnemonic = "start tree vicious crash drum meat turn price exile weasel slam hurt";
String path = "m/84'/1'/0'";
String xpub = "";
String xpubcore = "";
String descriptorcore = "";
String descriptorcorechange = "";

int first = 0;
int second = 0;
String address = "tb1q8wl9rq3ykk4yuf2zvh4h394lyvmp0ayp34mnqa"; // m/84'/1'/0'/0/0

String unsignedpsbt = "";
String signedpsbt = "";

String message = "Buy Bitcoin!";
String signature = "";

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
    uint16_t tbw, tbh;
    static int16_t lastX, lastY;
    static uint16_t lastW, lastH;
    static uint8_t hh = 0, mm = 0;
    static uint8_t lastDay = 0;
    static int16_t getX, getY;
    static uint16_t getW, getH;
    char buff[64] = "00:00";
    int battPerc, battPercView, battLoad;

    RTC_Date d = rtc->getDateTime();
    if (mm == d.minute && !fullScreen) {
        return ;
    }

    mm = d.minute;
    hh = d.hour;
    if (lastDay != d.day) {
        lastDay = d.day;
        fullScreen = true;  // repaint all
    }

    snprintf(buff, sizeof(buff), "%02d:%02d", hh, mm);

    if (fullScreen) {
        lastX = 25;
        lastY = 100;
        // Print logo
        ePaper->drawBitmap(5, 5, logoIcon, 134, 28, GxEPD_BLACK);

        // Print battery icon
        battLoad=0;
        if (twatch->power->isVBUSPlug())                // check if battery is charging
        {
          battLoad=1;
        }
        battPerc=twatch->power->getBattPercentage();    // get the status of the battery
        battPercView=((battPerc+20)/20);               // there are 10 different states to display the percentage of battery charge
        Serial.print(battPerc);
        Serial.print(battPercView);

        u8g2Fonts.setFont(u8g2_font_battery19_tn);
        u8g2Fonts.setCursor(175, 20);                // start writing at this position
        u8g2Fonts.setFontDirection(3);              // left to right (this is default)
        u8g2Fonts.print(battPercView);
        u8g2Fonts.setFontDirection(0);              // left to right (this is default)


        ePaper->drawFastHLine(10, 40, ePaper->width() - 20, GxEPD_BLACK);
        ePaper->drawFastHLine(10, 150, ePaper->width() - 20, GxEPD_BLACK);

        u8g2Fonts.setFont(u8g2_font_inr38_mn  ); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

        u8g2Fonts.setCursor(lastX, lastY);                // start writing at this position
        u8g2Fonts.print(buff);

        /* calculate the size of the box into which the text will fit */
        lastH = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
        lastW = u8g2Fonts.getUTF8Width(buff);

        u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312a); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

        tbh = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
        tbw = u8g2Fonts.getUTF8Width("SFYL Wallet");

        int16_t x, y;
        x = ((ePaper->width() - tbw) / 2) ;
        y = ((ePaper->height() - tbh) / 2) + 40  ;

        u8g2Fonts.setCursor(x, y);
        u8g2Fonts.print(buff);
        u8g2Fonts.setCursor(20, y + 50);
        u8g2Fonts.print("SFYL Wallet");
        getX = u8g2Fonts.getCursorX();
        getY = u8g2Fonts.getCursorY();
        getH  = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
        getW = u8g2Fonts.getUTF8Width("1000步");
        ePaper->update();


    } else {
        u8g2Fonts.setFont(u8g2_font_inr38_mn); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
        ePaper->fillRect(lastX, lastY - u8g2Fonts.getFontAscent() - 3, lastW, lastH, GxEPD_WHITE);
        ePaper->fillScreen(GxEPD_WHITE);
        ePaper->setTextColor(GxEPD_BLACK);
        lastW = u8g2Fonts.getUTF8Width(buff);
        u8g2Fonts.setCursor(lastX, lastY);
        u8g2Fonts.print(buff);
        ePaper->updateWindow(lastX, lastY - u8g2Fonts.getFontAscent() - 3, lastW, lastH, false);
    }
}

String processor(const String& var) {
  if (var == "MAC")          {
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    Serial.printf("Connected, mac address: %02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    return String(macAddr[0], HEX)+String(macAddr[1], HEX)+String(macAddr[2], HEX)+String(macAddr[3], HEX)+String(macAddr[4], HEX)+String(macAddr[5], HEX);
  }
  if (var == "BATTERY")      return String(twatch->power->getBattPercentage());
  if (var == "HEAP")         return String(ESP.getFreeHeap());
  if (var == "CHIPID")       {
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    uint32_t chipid = macAddressTrunc >> 40;
    return String(chipid, HEX);
  }
  if (var == "NETWORK")              return network;
  if (var == "MNEMONIC")             return mnemonic;
  if (var == "PATH")                 return path;
  if (var == "XPUB")                 return xpub;
  if (var == "XPUBCORE")             return xpubcore;
  if (var == "DESCRIPTORCORE")       return descriptorcore;
  if (var == "DESCRIPTORCORECHANGE") return descriptorcorechange;
  if (var == "FIRST")                return String(first);
  if (var == "SECOND")               return String(second);
  if (var == "ADDRESS")              return address;
  if (var == "UNSIGNEDPSBT")         return unsignedpsbt;
  if (var == "SIGNEDPSBT")           return signedpsbt;
  if (var == "MESSAGE")              return message;
  if (var == "SIGNATURE")            return signature;
  return String();
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

void setup() {
    Serial.begin(115200);
    delay(100);

    // Load spiffs
    if(!SPIFFS.begin()){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
    }

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

    // Initialize the interface
    mainPage(true);

    // Reduce CPU frequency
    setCpuFrequencyMhz(80);

    // Get secrets from spiffs
    if (!SPIFFS.exists("/secret.txt")){
      // create a spiffs file
      File file = SPIFFS.open("/secret.txt", FILE_WRITE);
      if (!file) {
          Serial.println("There was an error opening the file for writing");
          return;
      }
      String s = network+","+mnemonic+","+path;
      if (!file.print(s.c_str()))
          Serial.println("File write failed");
      file.close();
    } else { // read existing file
      File file = SPIFFS.open("/secret.txt", "r");
      if (!file) {
        Serial.println("Failed to open file for reading");
        return;
      }
      String fileContent;
      while (file.available()) {
        fileContent += char(file.read());
      }
      file.close();
      network = getValue(fileContent, ',', 0);
      mnemonic =  getValue(fileContent, ',', 1);
      path =  getValue(fileContent, ',', 2);
    }

    String pathcore = path;
    pathcore.replace("m","");
    pathcore.replace("'","h");

    // using empty password and derive main xprv
    HDPrivateKey root(mnemonic, "");
    // derive account according to BIP-84 for testnet
    HDPrivateKey account = root.derive(path);

    HDPublicKey xpub_key = account.xpub();
    xpub = "["+root.fingerprint()+pathcore+"]"+xpub_key.toString();

    HDPrivateKey account_core = root.derive(path);
    // avoid funny name for xpub
    account_core.type = UNKNOWN_TYPE;
    HDPublicKey xpub_key_core = account_core.xpub();
    xpubcore = xpub_key_core.toString();

    descriptorcore = "wpkh([";
    descriptorcore += root.fingerprint();
    descriptorcore += pathcore + "]";
    descriptorcore += xpubcore;
    descriptorcore += "/0/*)";
    descriptorcore += String("#")+descriptorChecksum(descriptorcore);

    descriptorcorechange = "wpkh([";
    descriptorcorechange += root.fingerprint();
    descriptorcorechange += pathcore + "]";
    descriptorcorechange += xpubcore;
    descriptorcorechange += "/1/*)";
    descriptorcorechange += String("#")+descriptorChecksum(descriptorcorechange);


    // derive first address
    HDPublicKey pub;
    pub = xpub_key.child(0).child(0);
    address = pub.segwitAddress(&Testnet);

    // AP
    String ssid = "SFYL_"+String(esp_random(), HEX);
    String password = String(esp_random(), HEX)+String(esp_random(), HEX);
    ssid = "ESP32";
    password = "12345678";
    WiFi.softAP(ssid.c_str(), password.c_str(), 1, 0, 1); // Max 1 connection
    delay(100);

    // Webserver
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });

    server.on("/address", HTTP_GET, [](AsyncWebServerRequest *request){
      String command_arg = "";
      if(request->hasArg("command"))
        command_arg = request->arg("command");
        if (command_arg == "getaddress"){
          first = request->arg("first").toInt();
          second = request->arg("second").toInt();

          HDPrivateKey root(mnemonic, "");
          HDPrivateKey account = root.derive(path);
          account.type = UNKNOWN_TYPE;
          HDPublicKey xpub = account.xpub();
          HDPublicKey pub = xpub.child(first).child(second);
          address = pub.segwitAddress(&Testnet);
        }
      request->send(SPIFFS, "/address.html", "text/html", false, processor);
    });

    server.on("/sign", HTTP_GET, [](AsyncWebServerRequest *request){
      String command_arg = "";
      if(request->hasArg("command"))
        command_arg = request->arg("command");
      if (command_arg == "psbt"){
        HDPrivateKey root(mnemonic, "");
        HDPrivateKey account = root.derive(path);
        unsignedpsbt = request->arg("unsignedpsbt");
        Serial.println(unsignedpsbt);
        PSBT psbt;
        psbt.parseBase64(unsignedpsbt);

        // going through all outputs
        for(int i=0; i<psbt.tx.outputsNumber; i++){
          Serial.print(psbt.tx.txOuts[i].address(&Testnet));
          Serial.print(" -> ");
          // You can also use .btcAmount() function that returns a float in whole Bitcoins
          Serial.print(int(psbt.tx.txOuts[i].amount));
          Serial.println(" sat");
        }

        /*// going through all outputs
        for(int i=0; i<psbt.tx.outputsNumber; i++){
          Serial.print(psbt.tx.txOuts[i].address(&Testnet));
          // check if it is a change output
          if(psbt.txOutsMeta[i].derivationsLen > 0){ // there is derivation path
            // considering only single key for simplicity
            HDPublicKey pub = hd.derive(der.derivation, der.derivationLen).xpub();
            // as pub is HDPublicKey it will also generate correct address type
            if(pub.address() == psbt.tx.txOuts[i].address()){
              Serial.print(" (change) ");
            }
          }
          Serial.print(" -> ");
          // You can also use .btcAmount() function that returns a float in whole Bitcoins
          Serial.print(int(psbt.tx.txOuts[i].amount));
          Serial.println(" sat");
        }*/

        psbt.sign(root);
        signedpsbt = psbt.toBase64();

      } else if (command_arg == "message"){
        HDPrivateKey root(mnemonic, "");
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
      if (command_arg == "setmnemonic"){
        mnemonic = request->arg("mnemonic");
      } else if (command_arg == "setpath"){
        path = request->arg("path");
      } else if (command_arg == "setnetwork"){
        network = request->arg("network");
      }
      // create a spiffs file
      File file = SPIFFS.open("/secret.txt", FILE_WRITE);
      if (!file) {
          Serial.println("There was an error opening the file for writing");
          return;
      }
      String s = network+","+mnemonic+","+path;
      if (!file.print(s.c_str()))
          Serial.println("File write failed");
      file.close();
      request->send(SPIFFS, "/settings.html", "text/html", false, processor);
    });

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
            //twatch->bl->isOn() ? twatch->bl->off() : twatch->bl->adjust(DEFAULT_BRIGHTNESS);

            // using empty password
            HDPrivateKey root(mnemonic, "");
            // now we can check how it is converted to xprv
            Serial.println(root);

            // derive account according to BIP-84 for testnet
            HDPrivateKey account = root.derive(path);
            // avoid funny name
            account.type = UNKNOWN_TYPE;
            // print account and it's xpub
            Serial.println(account);
            HDPublicKey xpub = account.xpub();
            Serial.println(xpub);

            // derive and print 5 different addresses
            HDPublicKey pub;
            for(int i=0; i<5; i++){
                // deriving in a different manner
                pub = xpub.child(0).child(i);
                Serial.println(pub.segwitAddress(&Testnet));
            }

            PSBT psbt;
            psbt.parseBase64("cHNidP8BAHECAAAAAbvujNVeCWqb0qj49V0FE4yNqzxNp/alDjJf6tfrDmLvAAAAAA"
                "D+////AgIMAwAAAAAAFgAUeVBepLdGFMTDXaEd8BcxXAjVXWSghgEAAAAAABYAFI0z1+/eJK2dc4lUc"
                "rmdIBEJbibsAAAAAAABAR/gkwQAAAAAABYAFBFV/d7WeO0DA8PtpNcne1avYrsIIgYCSUlKvX7bM2uW"
                "3icko9ATHDMcEV097AIjBJVvcYapQoAYpKDu61QAAIABAACAAAAAgAAAAAAAAAAAACICA0FKXUUYI9e"
                "Fz3Kf5iDa4Iz4fGUp1/a27bGx4zBNKl1mGKSg7utUAACAAQAAgAAAAIABAAAAAAAAAAAA");

            Serial.println("Transactions details:");
            Serial.println("Inputs");
            // going through all inputs
            for(int i=0; i<psbt.tx.inputsNumber; i++){
              //Serial.print(String(psbt.tx.txIns[i].hash));
            }
            Serial.println("Outputs:");
            // going through all outputs
            for(int i=0; i<psbt.tx.outputsNumber; i++){
              Serial.print(psbt.tx.txOuts[i].address(&Testnet));
              Serial.print(" -> ");
              // You can also use .btcAmount() function that returns a float in whole Bitcoins
              Serial.print(int(psbt.tx.txOuts[i].amount));
              Serial.println(" sat");
            }
            Serial.print("Fee: ");
            // Arduino can't print 64-bit ints so we need to convert it to int
            Serial.print(int(psbt.fee()));
            Serial.println(" sat");

            psbt.sign(root);
            Serial.println(psbt.toBase64());

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

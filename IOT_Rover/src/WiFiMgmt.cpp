//////////////////////////////// WiFiMgmt.cpp /////////////////////////////////
// Filename:	WiFiMgmt.cpp
// Description: Management for how the device connects to various Wifi networks
// Author:		Danon Bradford
// Date:		2018-12-26
//////////////////////////////// WiFiMgmt.cpp /////////////////////////////////

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <Arduino.h>
#include <String.h>
#include <ESP8266WiFi.h>
#include <Blynk/BlynkTimer.h>   
#include "Planque.h"
#include "Display.h"
#include "DeviceConfig.h"
#include "IDL_Version.h" 
#include "WiFiMgmt.h"

//*****************************************************************************
// Private Macro Definitions
//-----------------------------------------------------------------------------
#define PRINTLN(...) Serial.println(__VA_ARGS__)
// #define PRINTLN(...)
#define PRINT(...) Serial.print(__VA_ARGS__)
// #define PRINTLN(...)

#define CallbackFn_COUNT 4
#define DNS_PORT 53

//*****************************************************************************
// Private Constant Global Variables
//-----------------------------------------------------------------------------
static const char HTTP_HEAD_MAIN[] PROGMEM       = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{v}</title>";
static const char HTTP_STYLE[] PROGMEM           = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>";
static const char HTTP_SCRIPT[] PROGMEM          = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
static const char HTTP_HEAD_END[] PROGMEM        = "</head><body><div style='text-align:left;display:inline-block;min-width:260px;'>";
static const char HTTP_PORTAL_OPTIONS[] PROGMEM  = "<form action=\"/config\" method=\"get\"><button>Configure</button></form><br/><form action=\"/info\" method=\"get\"><button>Info</button></form><br/><form action=\"/exit\" method=\"post\"><button>Exit</button></form>";
static const char HTTP_ITEM[] PROGMEM            = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
static const char HTTP_FORM_START[] PROGMEM      = "<form method='get' action='configsave'>";
static const char HTTP_FORM_PARAM[] PROGMEM      = "<br/><input id='{i}' name='{n}' maxlength={l} placeholder='{p}' value='{v}' {c}>";
static const char HTTP_FORM_END[] PROGMEM        = "<br/><br/><br/><button type='submit'>Save</button></form>";
static const char HTTP_GO_BACK[] PROGMEM         = "<br/><div class=\"c\"><a href=\"/\">Back</a></div>";
static const char HTTP_SAVED[] PROGMEM           = "<div>Configuration Saved!<br/></div>";
static const char HTTP_END[] PROGMEM             = "</div></body></html>";

//*****************************************************************************
// Publicly Accessible Global Variable Definitions
//-----------------------------------------------------------------------------
WiFiMgmtClass WiFiMgmt;

//*****************************************************************************
// Externally Defined Global Variables
//-----------------------------------------------------------------------------
extern BlynkTimer GlobalTimer;

//*****************************************************************************
// Nonvolatile Memory (SPI Flash) "Planque"
//-----------------------------------------------------------------------------
#pragma pack(push)
#pragma pack(1)
typedef struct {
    uint16_t NvByteCount1;
    char NvSsidArray[4][32];
    char NvPassArray[4][32];
    uint16_t NvByteCount2;
}ConfigNV_t;
#pragma pack(pop)

static ConfigNV_t* WiFiInPlanque;

// Shortcuts to access the bytes that are stored in Planque (Nonvolatile Memory)
#define NV_ByteCount1               WiFiInPlanque->NvByteCount1
#define NV_ByteCount2               WiFiInPlanque->NvByteCount2
#define NV_SsidArray                WiFiInPlanque->NvSsidArray
#define NV_PassArray                WiFiInPlanque->NvPassArray

//*****************************************************************************
// Class Member Variable Definitions (static)
//-----------------------------------------------------------------------------
bool WiFiMgmtClass::StationConnected = false;
char* WiFiMgmtClass::SsidArrayNv[4];
char* WiFiMgmtClass::PassArrayNv[4];
ESP8266WiFiMulti WiFiMgmtClass::WifiMulti;
bool WiFiMgmtClass::_StayInSoftAP = true;
DNSServer WiFiMgmtClass::_DnsServer;
ESP8266WebServer WiFiMgmtClass::_WebServer(80);
WiFiMgmt_SubscriptionFn WiFiMgmtClass::_CallbackFns[CallbackFn_COUNT];
uint8_t WiFiMgmtClass::_CallbackCount = 0;
int WiFiMgmtClass::_WiFiTID;

//=============================================================================
// Class Member Method Definitions (static)
//-----------------------------------------------------------------------------
bool WiFiMgmtClass::Init() {

    // Allocate the same byte space for the WiFi credential bytes that are stored in Planque
    bool status = Planque.AllocateVars( (volatile void **)&WiFiInPlanque, (uint16_t)sizeof(*WiFiInPlanque) );

    // Disable WiFi.persistent - We have a better implementation.
    WiFi.persistent(false);

    // Check that there is valid data in the given Planque area. Fix if necessary.
    // Simply check the Nv_ByteCount at the head and tail!
    if (NV_ByteCount1 != sizeof(*WiFiInPlanque) || NV_ByteCount2 != sizeof(*WiFiInPlanque)) {   

        PRINTLN(F("Erase of WiFi credentials in planque detected!"));
        EraseNv();

        // Commit
        Planque.WriteBufferToFlash();
    }

    // Copy the addresses of the arrays for public usage.
    for (uint8_t i=0; i<4; i++) {
        SsidArrayNv[i] = NV_SsidArray[i];
        PassArrayNv[i] = NV_PassArray[i];
    }

    return status;
}

void WiFiMgmtClass::EraseNv() {
    NV_ByteCount1 = sizeof(*WiFiInPlanque);
    NV_ByteCount2 = sizeof(*WiFiInPlanque);

    // Set the SSID and PASS strings to just a null terminator
    NV_SsidArray[0][0] = '\0';
    NV_PassArray[0][0] = '\0';
    NV_SsidArray[1][0] = '\0';
    NV_PassArray[1][0] = '\0';
    NV_SsidArray[2][0] = '\0';
    NV_PassArray[2][0] = '\0';
    NV_SsidArray[3][0] = '\0';
    NV_PassArray[3][0] = '\0';
}

void WiFiMgmtClass::EnterSoftAP() {
    
    IPAddress apIP(1, 2, 3, 4);
    IPAddress apMSK(255, 255, 255, 0);

    PRINTLN(F("Configuring WiFi in access point mode..."));
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, apMSK);
    WiFi.softAP(DeviceConfig.ChipUidString.c_str(), DeviceConfig.ChipPwdString.c_str(), (int)random(1, 12), 0, 1);
    delay(500); // Without delay I've seen the IP address blank

    PRINT(F("AP IP address: "));
    PRINTLN(WiFi.softAPIP());

    PRINT(F("AP SSID: "));
    PRINTLN(WiFi.softAPSSID());

    PRINT(F("AP Password: "));
    PRINTLN(WiFi.softAPPSK());

    // Slowly show the WiFi Access Point SSID and Password on the display, as bright as possible.
    if (DeviceConfig.getDisplay()) {
        Display.SetScrollInterval(75);
        Display.SetBrightness(15);
        Display.SetString(Display_PRIMARY_Show, WiFi.softAPSSID() + "_" + WiFi.softAPPSK());
    }    

    // Setup the DNS server redirecting all the domains to the apIP
    _DnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    _DnsServer.start(DNS_PORT, "*", apIP);

    // Setup web pages
    _WebServer.on(String(F("/")), handleRoot);
    _WebServer.on(String(F("/generate_204")), handleRoot);     // Android captive portal. Maybe not needed. Might be handled by notFound handler.
    _WebServer.on(String(F("/fwlink")), handleRoot);           // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    _WebServer.onNotFound(handleNotFound);
    _WebServer.on(String(F("/config")), handleConfig);
    _WebServer.on(String(F("/configsave")), handleConfigSave);
    _WebServer.on(String(F("/info")), handleInfo);
    _WebServer.on(String(F("/exit")), handleExit);   
    _WebServer.on(String(F("/format")), handleFormat);   
        
    _WebServer.begin(); // Web server start
    PRINTLN(F("HTTP server started"));

    // Loop in here forever until we are done.
    while (_StayInSoftAP) {
        _DnsServer.processNextRequest();
        _WebServer.handleClient();
        GlobalTimer.run();
        
        // let the background os run
        yield(); 
    }
}

void WiFiMgmtClass::BeginStation() {

    WiFi.mode(WIFI_STA);

    // Read the wifi networks in planque, use them if they are good.
    for (uint8_t i=0; i<4; i++) {
        // PRINTLN("wifiNum: " + String(i, DEC) + " - ssid: " + NV_SsidArray[i] + " pass: " + NV_PassArray[i]);
        if (strlen(NV_SsidArray[i])) {
            WifiMulti.addAP(NV_SsidArray[i], NV_PassArray[i]); 
            // PRINTLN("installed!");
        }
    }

    // Add the hardcoded known wifi networks    
    // WifiMulti.addAP("UTS_WiFi", "secret");       // UTS Building WiFi - fake password
    WifiMulti.addAP("danon", "12345678");           // Backup availability

    if (DeviceConfig.getDisplay()) {
        Display.SetScrollInterval(50);
        Display.SetBrightness(7);

        String toDisplay = String(F("WiFi Scanning..."));
        if (ErrorCode_12SCheck(DeviceConfig.SetupError) == ErrorCode_FAIL) {
            toDisplay = String(F("0x")) + String(DeviceConfig.SetupError, HEX) + String(F(" ")) + toDisplay;
        }  
        Display.SetString(Display_PRIMARY_Show, toDisplay);
    }    

    _WiFiTID = GlobalTimer.setInterval(1000, WiFiRun);
}

bool WiFiMgmtClass::SubscribeStatus(WiFiMgmt_SubscriptionFn userFunction) {
    if (_CallbackCount >= CallbackFn_COUNT) {
        return false;
    }

    _CallbackFns[_CallbackCount++] = userFunction;

    return true;
}

// experimental
// bool WiFiMgmtClass::PowerDown() {
//     bool status = WiFi.mode( WIFI_OFF ) && WiFi.forceSleepBegin();
//     yield();
//     return status;
// }

// bool WiFiMgmtClass::PowerUp() {
//     bool status = WiFi.forceSleepWake();
//     yield();
//     return status && WiFi.mode(WIFI_STA);
// }

// private:
void WiFiMgmtClass::WiFiRun() {
    bool notify = false;

    if (WifiMulti.run() == WL_CONNECTED) {
        if (StationConnected == false) {
            // Announce that we are now connected to wifi :)
            PRINT(F("WiFi connected to "));
            PRINTLN(WiFi.SSID());
            if (DeviceConfig.getDisplay()) {
                String toDisplay = WiFi.SSID();
                if (ErrorCode_12SCheck(DeviceConfig.SetupError) == ErrorCode_FAIL) {
                    toDisplay = String(F("0x")) + String(DeviceConfig.SetupError, HEX) + String(F(" ")) + toDisplay;
                }                
                Display.SetString(Display_PRIMARY_Show, toDisplay);
            }  
            notify = true;
        }

        StationConnected = true;
        
    } else {
        PRINT(millis() / 1000);
        PRINTLN(F(" WiFi Scanning..."));

        if (StationConnected == true) {
            // Announce that we are now not connected to wifi :(
            PRINTLN(F("WiFi lost connection!"));
            notify = true;
        }

        StationConnected = false;
    }

    if (notify) {
        for (uint8_t i = 0; i < CallbackFn_COUNT; i++) {
            if (_CallbackFns[i]) {
                (*_CallbackFns[i])(StationConnected);
            }
        }
    }
}

// Is this an IP? 
boolean WiFiMgmtClass::isIp(String str) {
    for (size_t i = 0; i < str.length(); i++) {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9')) {
            return false;
        }
    }
    return true;
}

// IP to String? 
String WiFiMgmtClass::toStringIp(IPAddress ip) {
    String res = "";
    for (int i = 0; i < 3; i++) {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}

// Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again.
boolean WiFiMgmtClass::captivePortal() {
    if (!isIp(_WebServer.hostHeader()) ) {
        PRINTLN(F("Request redirected to captive portal"));
        _WebServer.sendHeader("Location", String("http://") + toStringIp(_WebServer.client().localIP()), true);
        _WebServer.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
        _WebServer.client().stop(); // Stop is needed because we sent no content length
        return true;
    }
    return false;
}

// Handle root or redirect to captive portal 
void WiFiMgmtClass::handleRoot() {
    PRINTLN(F("Handle root"));
    if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
        return;
    }

    String page = FPSTR(HTTP_HEAD_MAIN);
    page.replace("{v}", "IoT Data Logger");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_HEAD_END);
    page += String(F("<h1>"));
    page += WiFi.softAPSSID();
    page += String(F("</h1>"));
    page += String(F("<h3>")) + String(DeviceConfig.PersonNameNv) + String(F("</h3>"));
    page += String(F("<h3>UTS IoT Data Logger</h3>"));
    page += String(F("<h3>Configuration Manager</h3>"));
    page += FPSTR(HTTP_PORTAL_OPTIONS);
    page += FPSTR(HTTP_END);

    _WebServer.sendHeader("Content-Length", String(page.length()));
    _WebServer.send(200, "text/html", page);
}

void WiFiMgmtClass::handleNotFound() {
    if (captivePortal()) { // If captive portal redirect instead of displaying the error page.
        return;
    }

    String message = "File Not Found\n\n";
    message += "URI: ";
    message += _WebServer.uri();
    message += "\nMethod: ";
    message += ( _WebServer.method() == HTTP_GET ) ? "GET" : "POST";
    message += "\nArguments: ";
    message += _WebServer.args();
    message += "\n";

    for ( uint8_t i = 0; i < _WebServer.args(); i++ ) {
        message += " " + _WebServer.argName ( i ) + ": " + _WebServer.arg ( i ) + "\n";
    }
    _WebServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _WebServer.sendHeader("Pragma", "no-cache");
    _WebServer.sendHeader("Expires", "-1");
    _WebServer.sendHeader("Content-Length", String(message.length()));
    _WebServer.send ( 404, "text/plain", message );
}

// config page handler 
void WiFiMgmtClass::handleConfig() {

    String page = FPSTR(HTTP_HEAD_MAIN);
    page.replace("{v}", "Config Non-volatile Memory");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_HEAD_END);
    page += FPSTR(HTTP_FORM_START);

    page += F("<dt>WiFi 1 SSID / Password</dt>");
    String item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "ws1");
    item.replace("{n}", "ws1");
    item.replace("{p}", "WiFi 1 SSID");
    item.replace("{l}", "31");
    item.replace("{v}", NV_SsidArray[1]);
    page += item;
    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "wp1");
    item.replace("{n}", "wp1");
    item.replace("{p}", "WiFi 1 Password");
    item.replace("{l}", "31");
    item.replace("{v}", NV_PassArray[1]);
    page += item;
    page += "<br/><br/><br/>";
    
    page += F("<dt>WiFi 2 SSID / Password</dt>");
    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "ws2");
    item.replace("{n}", "ws2");
    item.replace("{p}", "WiFi 2 SSID");
    item.replace("{l}", "31");
    item.replace("{v}", NV_SsidArray[2]);
    page += item;
    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "wp2");
    item.replace("{n}", "wp2");
    item.replace("{p}", "WiFi 2 Password");
    item.replace("{l}", "31");
    item.replace("{v}", NV_PassArray[2]);
    page += item;
    page += "<br/><br/><br/>";
    
    page += F("<dt>WiFi 3 SSID / Password</dt>");
    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "ws3");
    item.replace("{n}", "ws3");
    item.replace("{p}", "WiFi 3 SSID");
    item.replace("{l}", "31");
    item.replace("{v}", NV_SsidArray[3]);
    page += item;
    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "wp3");
    item.replace("{n}", "wp3");
    item.replace("{p}", "WiFi 3 Password");
    item.replace("{l}", "31");
    item.replace("{v}", NV_PassArray[3]);
    page += item;
    page += "<br/><br/><br/>";

    page += F("<dt>Blynk Authentication Token</dt>");
    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "bl");
    item.replace("{n}", "bl");
    item.replace("{p}", "32_Character_String");
    item.replace("{l}", "32");
    item.replace("{v}", DeviceConfig.BlynkTokenNv);
    page += item;
    page += "<br/><br/><br/>";

    page += F("<dt>Your Name</dt>");
    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "pn");
    item.replace("{n}", "pn");
    item.replace("{p}", "Who is the device owner?");
    item.replace("{l}", "31");
    item.replace("{v}", DeviceConfig.PersonNameNv);
    page += item;
    page += "<br/><br/><br/>";

    page += F("<dt>Product Config Code</dt>");
    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "pc1");
    item.replace("{n}", "pc1");
    item.replace("{p}", "Hexadecimal number");
    item.replace("{l}", "8");
    item.replace("{v}", String(DeviceConfig.ProductConfigArrayNv[1], HEX));
    page += item;

    page += FPSTR(HTTP_FORM_END);
    page += FPSTR(HTTP_GO_BACK);
    page += FPSTR(HTTP_END);

    _WebServer.sendHeader("Content-Length", String(page.length()));
    _WebServer.send(200, "text/html", page);

    PRINTLN(F("Sent config page"));
}

// Handle the config save form and redirect to config page again */
void WiFiMgmtClass::handleConfigSave() {
    PRINTLN(F("config save"));

    bool writeNeeded = false;

    String page = FPSTR(HTTP_HEAD_MAIN);
    page.replace("{v}", "Configuration Saved");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_HEAD_END);
    page += F("<dl>");

    if (_WebServer.arg("ws1") != "") {
        writeNeeded |= Planque.NewCharArrayString(NV_SsidArray[1], _WebServer.arg("ws1"));
    }

    if (_WebServer.arg("wp1") != "") {
        writeNeeded |= Planque.NewCharArrayString(NV_PassArray[1], _WebServer.arg("wp1"));
    }

    if (_WebServer.arg("ws2") != "") {
        writeNeeded |= Planque.NewCharArrayString(NV_SsidArray[2], _WebServer.arg("ws2"));
    }

    if (_WebServer.arg("wp2") != "") {
        writeNeeded |= Planque.NewCharArrayString(NV_PassArray[2], _WebServer.arg("wp2"));
    }

    if (_WebServer.arg("ws3") != "") {
        writeNeeded |= Planque.NewCharArrayString(NV_SsidArray[3], _WebServer.arg("ws3"));
    }

    if (_WebServer.arg("wp3") != "") {
        writeNeeded |= Planque.NewCharArrayString(NV_PassArray[3], _WebServer.arg("wp3"));
    }

    if (_WebServer.arg("bl") != "") {
        writeNeeded |= Planque.NewCharArrayString(DeviceConfig.BlynkTokenNv, _WebServer.arg("bl"));
        if (_WebServer.arg("bl").length() != 32) {
            page += F("<dt>The Blynk Auth Token is not exactly 32 characters... Are you sure this is correct?</dt>");
            DeviceConfig.ValidBlynk = false;
        } else {
            DeviceConfig.ValidBlynk = true;
        }
    } else {
        page += F("<dt>The Blynk Auth Token is empty... Currently there is no way to communicate with your smartphone app...</dt>");
    }

    if (_WebServer.arg("pn") != "") {
        writeNeeded |= Planque.NewCharArrayString(DeviceConfig.PersonNameNv, _WebServer.arg("pn"));
    } else {
        page += F("<dt>The Your Name is empty... Please go back and fill in your name!</dt>");
    }

    if (_WebServer.arg("pc1") != "") {
        writeNeeded |= Planque.NewU32(&DeviceConfig.ProductConfigArrayNv[1], strtoul(_WebServer.arg("pc1").c_str(), NULL, 16));

        if (DeviceConfig.ProductConfigArrayNv[1] == 0) {
            page += F("<dt>The Product Config Code is 0... Are you sure this is correct?</dt>");
        }
    } else {
        page += F("<dt>The Product Config Code is empty... Please go back and enter the code!</dt>");
    }

    if (writeNeeded) {
        Planque.WriteBufferToFlash();
    }

    page += FPSTR(HTTP_SAVED);
    page += F("</dl>");
    page += FPSTR(HTTP_GO_BACK);
    page += FPSTR(HTTP_END);

    _WebServer.sendHeader("Content-Length", String(page.length()));
    _WebServer.send(200, "text/html", page);

    PRINTLN(F("Sent config save page"));
}

// Handle the info page 
void WiFiMgmtClass::handleInfo() {
    PRINTLN(F("Info"));

    String page = FPSTR(HTTP_HEAD_MAIN);
    page.replace("{v}", "Info");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_HEAD_END);
    page += F("<dl>");

    page += F("<dt>Device Hardware Version</dt><dd>");
    page += String(IDL_Version_Hardware);
    page += F("</dd>");

    page += F("<dt>Device Software Version</dt><dd>");
    page += String(IDL_Version_Software);
    page += F("</dd>");

    page += F("<dt>Device UID</dt><dd>");
    page += DeviceConfig.ChipUidString;
    page += F("</dd>");

    page += F("<dt>Device Password</dt><dd>");
    page += DeviceConfig.ChipPwdString;
    page += F("</dd>");

    page += F("<dt>Device Setup Error</dt><dd>");
    page += String(F("0x")) + String(DeviceConfig.SetupError, HEX);
    page += F("</dd>");

    // ADC is not configured for this.
    // page += F("<dt>ESP8266 VCC</dt><dd>");
    // page += ESP.getVcc();
    // page += F("</dd>");

    page += F("<dt>ESP8266 Free Heap</dt><dd>");
    page += ESP.getFreeHeap();
    page += F(" bytes</dd>");

    page += F("<dt>ESP8266 SDK Version</dt><dd>");
    page += ESP.getSdkVersion();
    page += F("</dd>");

    page += F("<dt>ESP8266 Core Version</dt><dd>");
    page += ESP.getCoreVersion();
    page += F("</dd>");

    page += F("<dt>ESP8266 Full Version</dt><dd>");
    page += ESP.getFullVersion();
    page += F("</dd>");

    page += F("<dt>ESP8266 Boot Version</dt><dd>");
    page += ESP.getBootVersion();
    page += F("</dd>");

    page += F("<dt>ESP8266 Boot Mode</dt><dd>");
    page += ESP.getBootMode();
    page += F("</dd>");

    page += F("<dt>ESP8266 CPU Frequency</dt><dd>");
    page += ESP.getCpuFreqMHz();
    page += F(" MHz</dd>");

    page += F("<dt>Flash Chip ID</dt><dd>");
    page += String(ESP.getFlashChipId(),HEX);
    page += F("</dd>");

    page += F("<dt>Flash Chip Real Size</dt><dd>");
    page += ESP.getFlashChipRealSize();
    page += F(" bytes</dd>");

    page += F("<dt>Flash Chip Size</dt><dd>");
    page += ESP.getFlashChipSize();
    page += F(" bytes</dd>");

    page += F("<dt>Flash Chip Speed</dt><dd>");
    page += ESP.getFlashChipSpeed() / (uint32_t)1e6;
    page += F(" MHz</dd>");

    page += F("<dt>Flash Chip Mode</dt><dd>");
    page += ESP.getFlashChipMode();
    page += F("</dd>");

    page += F("<dt>Flash Chip Size By Chip Id</dt><dd>");
    page += ESP.getFlashChipSizeByChipId();
    page += F("</dd>");  

    page += F("<dt>Sketch Size</dt><dd>");
    page += ESP.getSketchSize();
    page += F(" bytes</dd>");

    page += F("<dt>Sketch MD5</dt><dd>");
    page += ESP.getSketchMD5();
    page += F("</dd>");

    page += F("<dt>Free Sketch Space</dt><dd>");
    page += ESP.getFreeSketchSpace();
    page += F(" bytes</dd>");

    page += F("<dt>Reset Reason</dt><dd>");
    page += ESP.getResetReason();
    page += F("</dd>");

    page += F("<dt>Reset Info</dt><dd>");
    page += ESP.getResetInfo();
    page += F("</dd>");

    page += F("<dt>Cycle Count</dt><dd>");
    page += ESP.getCycleCount();
    page += F("</dd>");

    page += F("<dt>Soft AP IP</dt><dd>");
    page += WiFi.softAPIP().toString();
    page += F("</dd>");

    page += F("<dt>Soft AP MAC</dt><dd>");
    page += WiFi.softAPmacAddress();
    page += F("</dd>");

    page += F("<dt>Station MAC</dt><dd>");
    page += WiFi.macAddress();
    page += F("</dd>");

    page += F("</dl>");
    page += FPSTR(HTTP_GO_BACK);
    page += FPSTR(HTTP_END);

    _WebServer.sendHeader("Content-Length", String(page.length()));
    _WebServer.send(200, "text/html", page);

    PRINTLN(F("Sent info page"));
}

// Handle the exit page
void WiFiMgmtClass::handleExit() {
    PRINTLN(F("Exit"));

    String page = FPSTR(HTTP_HEAD_MAIN);
    page.replace("{v}", "Exit");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_HEAD_END);
    page += F("Module will exit SoftAP mode and now try to connect to the known WiFi Networks");
    page += FPSTR(HTTP_END);

    _WebServer.sendHeader("Content-Length", String(page.length()));
    _WebServer.send(200, "text/html", page);

    _StayInSoftAP = false;

    PRINTLN(F("Sent Exit page"));
}

// Handle the Format page
void WiFiMgmtClass::handleFormat() {
    PRINTLN(F("Format"));

    EraseNv();
    DeviceConfig.EraseNv();

    // Commit
    Planque.WriteBufferToFlash();

    String page = FPSTR(HTTP_HEAD_MAIN);
    page.replace("{v}", "Format");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_HEAD_END);
    page += F("The Non-volatile Memory has been erased.");
    page += FPSTR(HTTP_GO_BACK);
    page += FPSTR(HTTP_END);

    _WebServer.sendHeader("Content-Length", String(page.length()));
    _WebServer.send(200, "text/html", page);

    PRINTLN(F("Sent Format page"));
}

// WiFiMgmt.cpp EOF

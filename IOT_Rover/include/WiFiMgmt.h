//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH WiFiMgmt.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Filename:	WiFiMgmt.h
// Description: Management for how the device connects to various Wifi networks
// Author:		Danon Bradford
// Date:		2018-12-26
//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH WiFiMgmt.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
#ifndef WiFiMgmt_h
#define WiFiMgmt_h

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <stdint.h>
#include <ESP8266WiFiMulti.h> 
#include <DNSServer.h>
#include <ESP8266WebServer.h>

//=============================================================================
// Public Structure's & Type Definitions
//-----------------------------------------------------------------------------
typedef void (*WiFiMgmt_SubscriptionFn)(const bool status);

//=============================================================================
// Class Declaration
//-----------------------------------------------------------------------------
class WiFiMgmtClass {
    public:
    WiFiMgmtClass() {}; // Constructor
    static bool StationConnected;
    static char* SsidArrayNv[4];
    static char* PassArrayNv[4];
    static ESP8266WiFiMulti WifiMulti;
    static bool Init();
    static void EraseNv();
    static void EnterSoftAP();
    static void BeginStation();
    static bool SubscribeStatus(WiFiMgmt_SubscriptionFn userFunction); 

    private:
    static bool _StayInSoftAP;
    static DNSServer _DnsServer;
    static ESP8266WebServer _WebServer;
    static WiFiMgmt_SubscriptionFn _CallbackFns[];
    static uint8_t _CallbackCount;
    static int _WiFiTID;
    static void WiFiRun();
    static boolean isIp(String str); 
    static String toStringIp(IPAddress ip);
    static boolean captivePortal();
    static void handleRoot();
    static void handleNotFound();
    static void handleConfig();
    static void handleConfigSave();  
    static void handleInfo();
    static void handleExit();
    static void handleFormat();
};

//=============================================================================
// Global Instance Declarations (Publicly Accessible)
//-----------------------------------------------------------------------------
extern WiFiMgmtClass WiFiMgmt;

#endif /* WiFiMgmt_h */

// WiFiMgmt.h EOF
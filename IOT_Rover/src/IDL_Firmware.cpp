// IDL Firmware

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include "IDL_Version.h"
#include "ErrorCode.h"
#include "Planque.h"
#include "WiFiMgmt.h"
#include "DeviceConfig.h"
#include "Display.h"
#include "Sensors.h"
#include "Mobility.h"
#include "VirtualPinDefs.h"

//*****************************************************************************
// Private Macro Definitions
//-----------------------------------------------------------------------------
#define SwitchA_PIN             15
#define SwitchA_CHANGE          RISING
#define SwitchB_PIN             0
#define SwitchB_CHANGE          FALLING

//*****************************************************************************
// Private Global Variables
//-----------------------------------------------------------------------------
// You are welcome to continue to use our Blynk server!
// It has a substantial amount of more energy than the offical Blynk server!
const char Blynk_Server[] = "iot.nortcele.win";
const uint16_t Blynk_Port = 8080;

// Switches
volatile bool SwPressA = false;
volatile bool SwPressB = false;
bool ToggleStateA = false;
bool ToggleStateB = false;

// Display
uint8_t MenuDisplayMode = 0x00;
uint32_t TemporaryTimeout = 3000;

// Blynk timer 
BlynkTimer GlobalTimer; 
int PushServerTID = -1;

// Push data to the Blynk server configuration
const uint32_t DefaultPushInterval = 10000;
const uint8_t ThingsToPush = 10;

//*****************************************************************************
// Private Function Declarations
//-----------------------------------------------------------------------------
void NotifyBlynk(const bool status);
void SwitchA_Callback(void);
void SwitchB_Callback(void);
void Switch_Handler(void);
void PushDataToBlynkServer(void);

//=============================================================================
// Arduino Function Definitions
//-----------------------------------------------------------------------------
void setup() {

    // Debug console
    Serial.begin(115200);
    Serial.println("\n\rIDL Firmware Boot!");
    Serial.println(IDL_Version_Software);
    
    // Init usage of non volatile memory.
    Planque.Init();

    // Init Wifi, allocate the same Planque location.
    WiFiMgmt.Init();

    // Init DeviceConfig, allocate the same Planque location.
    DeviceConfig.Init();

    // Setup Switch A and Switch B.
    pinMode(SwitchA_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(SwitchA_PIN), SwitchA_Callback, SwitchA_CHANGE);
    pinMode(SwitchB_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(SwitchB_PIN), SwitchB_Callback, SwitchB_CHANGE);

    // Initialise wire. The I2C bus is used for the Display module and Accelerometer / Pedometer.
    Wire.begin();

    bool success;

    // Initialise the Display module    
    if (DeviceConfig.getDisplay() == DC_Display_HT16K33) {
        success = Display.Init();
        ErrorCode_12SLog(&DeviceConfig.SetupError, !success);
    }

    // Initialise the sensors
    Sensors.Init(&DeviceConfig.SetupError);
    
    // Read Switch A and Switch B. Enter Soft AP mode if both are pressed down.
    if (digitalRead(SwitchA_PIN) && !digitalRead(SwitchB_PIN)) {
        WiFiMgmt.EnterSoftAP();
    }

    if (DeviceConfig.getPower() == DC_Power_UploadThenDeepSleep && Sensors.GetDhtPin() != 2u) {
        // Blue LED on
        pinMode(2, OUTPUT);
        digitalWrite(2, LOW);
    }

    // Periodically push the sensor data to the Blynk server
    PushServerTID = GlobalTimer.setInterval(DefaultPushInterval / ThingsToPush, PushDataToBlynkServer);

    // Connect to known wifi networks!    
    WiFiMgmt.BeginStation();

    // Configure connecting to the Blynk server by subscribing to WiFi connection status.
    WiFiMgmt.SubscribeStatus(NotifyBlynk);
}

void loop() {
    GlobalTimer.run();    
    Switch_Handler();

    if (WiFiMgmt.StationConnected && DeviceConfig.ValidBlynk) {
        Blynk.run();        
    } 
}

//=============================================================================
// Blynk Function Definitions
//-----------------------------------------------------------------------------

// We have juct connected to the Blynk server!
BLYNK_CONNECTED() {

    if (DeviceConfig.getPower() == DC_Power_EverythingAlwaysOn) {   
        Blynk.setProperty(DisplayMode_Vpin, "labels", "Text", "Number", "U64", "Show Sensor", "Joystick", "Joystick (Persistent)", "All LED's On", "All LED's Off", "Display off");
        Blynk.syncVirtual(DisplayMode_Vpin, Brightness_Vpin, ScrollRate_Vpin, ScrollEnable_Vpin, TempTimeout_Vpin, PushPeriod_Vpin, PushEnable_Vpin, TempOffset_Vpin, HumOffset_Vpin, MobilityFLAddr_Vpin, MobilityFRAddr_Vpin);

        Blynk.virtualWrite(SwitchA_Vpin, 255*ToggleStateA);
        Blynk.virtualWrite(SwitchB_Vpin, 255*ToggleStateB);

        Blynk.virtualWrite(ChipUID_Vpin, String(ESP.getChipId(),HEX));
        Blynk.virtualWrite(WiFiName_Vpin, WiFi.SSID());
        Blynk.virtualWrite(WiFiRSSI_Vpin, WiFi.RSSI());
        Blynk.virtualWrite(LocalIP_Vpin, WiFi.localIP().toString());
    } else if (DeviceConfig.getPower() == DC_Power_UploadThenDeepSleep) {
        // Push it to the server
        Blynk.virtualWrite(Temperature_Vpin, Sensors.GetTemperature());
        Blynk.virtualWrite(Humidity_Vpin, Sensors.GetHumidity());   
        Blynk.virtualWrite(BatteryVoltage_Vpin, Sensors.GetBatteryVoltage()); 
        Blynk.virtualWrite(LightLux_Vpin, Sensors.GetLightLux()); 

        // Go into deep sleep for a 60 secs.
        WiFi.mode( WIFI_OFF );
        WiFi.forceSleepBegin();
        delay(1);
        if (Sensors.GetDhtPin() != 2u) {
            // Blue LED off
            digitalWrite(2, HIGH);
        }
        ESP.deepSleep(60e6);
    }
}

// Android/iPhone app has given us a new default display mode!
BLYNK_WRITE(DisplayMode_Vpin) {
    switch (param.asInt()) {
    
        //  Text Mode
        case 1: {
            MenuDisplayMode = 0x00;
            Display.SetMode(Display_PRIMARY_Show, Display_String_Mode);
            Blynk.syncVirtual(DefaultText_Vpin);
            break;
        }

        // Number Mode
        case 2:{
            MenuDisplayMode = 0x01;
            Display.SetMode(Display_PRIMARY_Show, Display_String_Mode);
            Blynk.syncVirtual(DefaultNumber_Vpin);
            break;
        }

        // U64 Mode
        case 3:{
            MenuDisplayMode = 0x02;
            Display.SetMode(Display_PRIMARY_Show, Display_U64_Mode);
            Blynk.syncVirtual(DefaultU64_Vpin);
            break;
        }

        // Show Sensor Mode
        case 4:{
            MenuDisplayMode = 0x03;
            Display.SetMode(Display_PRIMARY_Show, Display_String_Mode);
            Blynk.syncVirtual(DefaultSensor_Vpin);
            break;
        }

        // Joystick Mode
        case 5:{
            MenuDisplayMode = 0x04;
            Display.SetMode(Display_PRIMARY_Show, Display_Manual_Mode);
            Display.Clear();
            Display.WriteBuffer();
            break;
        }

        // Joystick (Persistent) Mode
        case 6:{
            MenuDisplayMode = 0x05;
            Display.SetMode(Display_PRIMARY_Show, Display_Manual_Mode);
            Display.Clear();
            Display.WriteBuffer();
            break;
        }

        // All LED's On Mode
        case 7:{
            MenuDisplayMode = 0x06;
            Display.SetMode(Display_PRIMARY_Show, Display_AllLedsOn_Mode);
            break;
        }

        // All LED's Off Mode
        case 8:{
            MenuDisplayMode = 0x07;
            Display.SetMode(Display_PRIMARY_Show, Display_AllLedsOff_Mode);
            break;
        }

        // Display Off Mode
        case 9:{
            MenuDisplayMode = 0x08;
            Display.SetMode(Display_PRIMARY_Show, Display_DisplayOff_Mode);
            break;
        }
    }

    // Tell the sensor methods to stop setting the primary display string.
    if (MenuDisplayMode != 0x03)
        Sensors.ShowOnDisplayPrimary = 0x00;
}

// Android/iPhone app is giving us a new string of text to display by default.
BLYNK_WRITE(DefaultText_Vpin) {
    if (!param.isEmpty() && MenuDisplayMode == 0x00)
        Display.SetString(Display_PRIMARY_Show, param.asString());
}

// Android/iPhone app is giving us a new number to display by default.
BLYNK_WRITE(DefaultNumber_Vpin) {
    if (!param.isEmpty() && MenuDisplayMode == 0x01)
        Display.SetNumber(Display_PRIMARY_Show, param.asInt());
}

// Android/iPhone app is giving us new U64's to display by default.
BLYNK_WRITE(DefaultU64_Vpin) {
    if (param.getLength() >= 16 && MenuDisplayMode == 0x02) {
        uint8_t charLength = param.getLength();
        uint8_t u64Count = 0;
        while (charLength >= 16) {
            uint8_t start = u64Count * 16;
            uint64_t image = strtoull(String(param.asString()).substring(start, start+16).c_str(), NULL, 16);
            Display.SetU64Image(Display_PRIMARY_Show, image, u64Count++);
            charLength -= 16;
        }
        Display.SetU64Count(Display_PRIMARY_Show, u64Count);
    }
}

// Android/iPhone app is giving us a sensor ID that we need to display.
BLYNK_WRITE(DefaultSensor_Vpin) {
    if (!param.isEmpty() && MenuDisplayMode == 0x03)
        Sensors.ShowOnDisplayPrimary = param.asInt();
}

// Android/iPhone app is giving us a new brightness level to display.
BLYNK_WRITE(Brightness_Vpin) {
    if (!param.isEmpty())
        Display.SetBrightness(param.asInt());
}

// Android/iPhone app is giving us a new scroll rate.
BLYNK_WRITE(ScrollRate_Vpin) {
    if (!param.isEmpty() && param.asInt())
        Display.SetScrollInterval(1000 / param.asInt());
}

// Android/iPhone app is telling us to enable/disable the scrolling.
BLYNK_WRITE(ScrollEnable_Vpin) {
    // Only Menu Display modes 0x00, 0x01, 0x02 or 0x03 use the scrolling.
    if (!param.isEmpty()  ) 
        Display.ScrollEnable(param.asInt());
}

// Android/iPhone app is giving us a new string of text to display temporarily .
BLYNK_WRITE(TempText_Vpin) {
    if (!param.isEmpty()  ) {
        Display.SetString(Display_TEMPORARY_Show, param.asString());
        Display.SetMode(Display_TEMPORARY_Show, Display_String_Mode);
        Display.ActivateTempShow(TemporaryTimeout);
    }
}

// Android/iPhone app is giving us a new number to display temporarily.
BLYNK_WRITE(TempNumber_Vpin) {
    if (!param.isEmpty() ) {
        Display.SetNumber(Display_TEMPORARY_Show, param.asInt());
        Display.SetMode(Display_TEMPORARY_Show, Display_String_Mode);
        Display.ActivateTempShow(TemporaryTimeout);
    }
}

// Android/iPhone app is giving us new U64's to display temporarily.
BLYNK_WRITE(TempU64_Vpin) {
    if (param.getLength() >= 16 ) {
        uint8_t charLength = param.getLength();
        uint8_t u64Count = 0;
        while (charLength >= 16) {
            uint8_t start = u64Count * 16;
            uint64_t image = strtoull(String(param.asString()).substring(start, start+16).c_str(), NULL, 16);
            Display.SetU64Image(Display_TEMPORARY_Show, image, u64Count++);
            charLength -= 16;
        }
        Display.SetMode(Display_TEMPORARY_Show, Display_U64_Mode);
        Display.SetU64Count(Display_TEMPORARY_Show, u64Count);
        Display.ActivateTempShow(TemporaryTimeout);
    }
}

// Android/iPhone app is giving us a sensor ID that we need to display temporarily.
BLYNK_WRITE(TempSensor_Vpin) {
    if (!param.isEmpty() )
        Sensors.ShowOnDisplay(Display_TEMPORARY_Show, param.asInt());
        Display.SetMode(Display_TEMPORARY_Show, Display_String_Mode);
        Display.ActivateTempShow(TemporaryTimeout);
}

// Android/iPhone app is giving us a timeout for a temporary display.
BLYNK_WRITE(TempTimeout_Vpin) {
    if (!param.isEmpty() && param.asInt())
        TemporaryTimeout = param.asInt() * 1000;
}

// Android/iPhone app is giving us the command to show something temporarily on the display.
BLYNK_WRITE(TempShowNow_Vpin) {
    if (!param.isEmpty() && param.asInt())
        Display.ActivateTempShow(TemporaryTimeout);
}

// Android/iPhone app is giving us new joystick location information.
BLYNK_WRITE(JoystickInput_Vpin) {
    if (!param.isEmpty() && (MenuDisplayMode == 0x04 || MenuDisplayMode == 0x05)) {
        if (MenuDisplayMode == 0x04)
            Display.Clear();

        Display.SetPixel(param[0].asInt(), param[1].asInt(), 0x01);
        Display.WriteBuffer();
    }
}

// Android/iPhone app is giving us a new period to push data to the server.
BLYNK_WRITE(PushPeriod_Vpin) {
    if (!param.isEmpty() && param.asInt())
        (void)GlobalTimer.changeInterval(PushServerTID, param.asInt() * 1000 / ThingsToPush);
}

// Android/iPhone app is telling us to enable/disable data pushing to the server.
BLYNK_WRITE(PushEnable_Vpin) {
    if (!param.isEmpty()) {
        if (param.asInt())
            GlobalTimer.enable(PushServerTID);
        else
            GlobalTimer.disable(PushServerTID);
    } 
}

// Android/iPhone app is giving us a new temperature offset.
BLYNK_WRITE(TempOffset_Vpin) {
    if (!param.isEmpty())
        Sensors.SetTemperatureOffset(param.asFloat());
}

// Android/iPhone app is giving us a new humidity offset.
BLYNK_WRITE(HumOffset_Vpin) {
    if (!param.isEmpty())
        Sensors.SetHumidityOffset(param.asFloat());
}

// New Mobility Stuff
// Work in progress
// Develop work
// There appears to be an android Joystick bug.
// Regarding write interval.
// There is no write interval on the IOS app. This seems to work fine.

BLYNK_WRITE(MobilityStatus_Vpin) { 
    Serial.println("MobilityStatus_Vpin"); 
    Mobility.PrintMotorFaults();
}

// Joystick
BLYNK_WRITE(MobilityJoy_Vpin) {
    if (!param.isEmpty()) {
        int8_t steer = param[0].asInt();
        int8_t speed = param[1].asInt();

        Mobility.SetDrive(speed, steer);
    }
}

int8_t Temp_Dev_Speed = 0;
int8_t Temp_Dev_Steer = 0;

// Speed
BLYNK_WRITE(MobilitySpeed_Vpin) { 
    if (!param.isEmpty()){ 
        Temp_Dev_Speed = param.asInt();

        Mobility.SetDrive(Temp_Dev_Speed, Temp_Dev_Steer);
    } 
} 

// Steer
BLYNK_WRITE(MobilitySteer_Vpin) { 
    if (!param.isEmpty()){ 
        Temp_Dev_Steer = param.asInt();

        Mobility.SetDrive(Temp_Dev_Speed, Temp_Dev_Steer);
    } 
}

// Front Left Address
BLYNK_WRITE(MobilityFLAddr_Vpin) {
    if (!param.isEmpty() && param.asInt() <= DRV8830_Addr8 &&
        param.asInt() >= DRV8830_Addr0) {
        Mobility.SetMotorAddr(Mobility_FrontLeftMotor,
                              (DRV8830_Address)param.asInt());
    }
}

// Front Right Address
BLYNK_WRITE(MobilityFRAddr_Vpin) {
    if (!param.isEmpty() && param.asInt() <= DRV8830_Addr8 &&
        param.asInt() >= DRV8830_Addr0) {
        Mobility.SetMotorAddr(Mobility_FrontRightMotor,
                              (DRV8830_Address)param.asInt());
    }
}

//*****************************************************************************
// Private Function Definitions
//-----------------------------------------------------------------------------
void NotifyBlynk(const bool status) {
    if (status && DeviceConfig.ValidBlynk) {
        if (!Blynk.connected()) {
            Serial.print("Blynk Token: ");
            Serial.println(DeviceConfig.BlynkTokenNv);
            Blynk.config(DeviceConfig.BlynkTokenNv, Blynk_Server, Blynk_Port);
            Blynk.connect();
        }
    } else {
        Blynk.disconnect();
    }
}

ICACHE_RAM_ATTR void SwitchA_Callback(void) {
    SwPressA = true;
}

ICACHE_RAM_ATTR void SwitchB_Callback(void) {
    SwPressB = true;
}

void Switch_Handler(void) {
    if (SwPressA) {
        SwPressA = false;
        ToggleStateA = !ToggleStateA;
        Blynk.virtualWrite(SwitchA_Vpin, 255*ToggleStateA);
    }

    if (SwPressB) {
        SwPressB = false;
        ToggleStateB = !ToggleStateB;
        Blynk.virtualWrite(SwitchB_Vpin, 255*ToggleStateB);
    }    
}

void PushDataToBlynkServer(void) {
    static uint8_t state = 0;

    switch(state++) {
        case 0: Blynk.virtualWrite(Temperature_Vpin, Sensors.GetTemperature());
            break;
        case 1: Blynk.virtualWrite(Humidity_Vpin, Sensors.GetHumidity());;
            break;
        case 2: Blynk.virtualWrite(HeatIndex_Vpin, Sensors.GetHeatIndex());
            break;
        case 3: Blynk.virtualWrite(DewPoint_Vpin, Sensors.GetDewPoint());
            break;
        case 4: Blynk.virtualWrite(BatteryVoltage_Vpin, Sensors.GetBatteryVoltage());
            break;
        case 5: Blynk.virtualWrite(LightLux_Vpin, Sensors.GetLightLux());
            break;
        case 6: Blynk.virtualWrite(StepCount_Vpin, Sensors.StepCount);
            break;
        case 7: Blynk.virtualWrite(Orientation_Vpin, Sensors.Orientation);
            break;
        case 8: Blynk.virtualWrite(UpTimeRead_Vpin, (millis() / 1000));
            break;
        case 9: 
            Blynk.virtualWrite(WiFiName_Vpin, WiFi.SSID());
            Blynk.virtualWrite(WiFiRSSI_Vpin, WiFi.RSSI());
            Blynk.virtualWrite(LocalIP_Vpin, WiFi.localIP().toString());
            break; 
        default: break;
    }

    if (state >= ThingsToPush)
        state = 0;    
}
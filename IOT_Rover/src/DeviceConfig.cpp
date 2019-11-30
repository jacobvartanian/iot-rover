//////////////////////////////// DeviceConfig.cpp /////////////////////////////
// Filename:	DeviceConfig.cpp
// Description: Device Configuration
// Author:		Danon Bradford
// Date:		2018-12-01
//////////////////////////////// DeviceConfig.cpp /////////////////////////////

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <Arduino.h>
#include <Blynk/BlynkTimer.h>   
#include "Planque.h"
#include "DeviceConfig.h"

//*****************************************************************************
// Private Macro Definitions
//-----------------------------------------------------------------------------
#define PRINTLN(...) Serial.println(__VA_ARGS__)
// #define PRINTLN(...)
#define PRINT(...) Serial.print(__VA_ARGS__)
// #define PRINT(...)

//*****************************************************************************
// Publicly Accessible Global Variable Definitions
//-----------------------------------------------------------------------------
DeviceConfigClass DeviceConfig;

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
    uint32_t ProductConfigArray[4];     // Needs to be aligned correctly!
    uint16_t NvByteCount1;
    char NvPersonName[32];
    char NvBlynkToken[33];
    uint8_t WastedBytes[3];             // ESP8266 is 4 byte aligned.
    uint16_t NvByteCount2;              // This is why the order is different. 
}ConfigNV_t;                            
#pragma pack(pop)

static ConfigNV_t* DevCfgPlanque;

// Shortcuts to access the bytes that are stored in Planque (Nonvolatile Memory)
#define NV_ByteCount1               DevCfgPlanque->NvByteCount1
#define NV_ByteCount2               DevCfgPlanque->NvByteCount2
#define NV_BlynkToken               DevCfgPlanque->NvBlynkToken
#define NV_PersonName               DevCfgPlanque->NvPersonName
#define NV_ProductConfigArray       DevCfgPlanque->ProductConfigArray

//*****************************************************************************
// Class Member Variable Definitions (static)
//-----------------------------------------------------------------------------
EC_12S_t DeviceConfigClass::SetupError = ErrorCode_NONE;
uint32_t DeviceConfigClass::ChipUid;
uint32_t DeviceConfigClass::ChipPwd;
String DeviceConfigClass::ChipUidString;
String DeviceConfigClass::ChipPwdString;
char* DeviceConfigClass::BlynkTokenNv;
bool DeviceConfigClass::ValidBlynk = false;
char* DeviceConfigClass::PersonNameNv;
uint32_t* DeviceConfigClass::ProductConfigArrayNv;

//=============================================================================
// Class Member Method Definitions (static)
//-----------------------------------------------------------------------------
bool DeviceConfigClass::Init() {

    ChipUid = ESP.getChipId();
    ChipUidString = String(ChipUid,HEX);

    ChipPwd = ReverseBitsU32(ChipUid);
    ChipPwdString = String(ChipPwd);

    // while (ChipUidString.length() < 8) {
    //     ChipUidString = String("0") + ChipUidString;
    // }

    while (ChipPwdString.length() < 8) {
        ChipPwdString = String("0") + ChipPwdString;
    }

    // Keep the passwrod length at 8 digits
    if (ChipPwdString.length() > 8) {
        ChipPwdString.remove(8);
    }

    PRINT(F("Device Unique ID: "));
    PRINTLN(ChipUidString);

    // PRINT(F("Device Password: "));
    // PRINTLN(ChipPwdString);

    // Allocate the same byte space for the device configuration bytes that are stored in Planque
    bool status = Planque.AllocateVars( (volatile void **)&DevCfgPlanque, (uint16_t)sizeof(*DevCfgPlanque) );

    // Check that there is valid data in the given Planque area. Fix if necessary.
    // Simply check the Nv_ByteCount at the head and tail!
    if (NV_ByteCount1 != sizeof(*DevCfgPlanque) || NV_ByteCount2 != sizeof(*DevCfgPlanque)) {   

        PRINTLN(F("Erase of device config in planque detected!"));
        EraseNv();

        // Commit
        Planque.WriteBufferToFlash();
    }

    // Link the data for public use
    BlynkTokenNv = NV_BlynkToken;
    PersonNameNv = NV_PersonName;
    ProductConfigArrayNv = &(NV_ProductConfigArray[0]);

    // Check the validity of the Blynk Authentication Token
    if (strlen(DeviceConfig.BlynkTokenNv) == 32) {
        ValidBlynk = true;
    }

    return status;
}

void DeviceConfigClass::EraseNv() {
    NV_ByteCount1 = sizeof(*DevCfgPlanque);
    NV_ByteCount2 = sizeof(*DevCfgPlanque);

    // Set the data to just a null terminator or zero
    NV_BlynkToken[0] = '\0';
    NV_PersonName[0] = '\0';
    NV_ProductConfigArray[0] = 0;
    NV_ProductConfigArray[1] = DC_PC1_Default;
    NV_ProductConfigArray[2] = 0;
    NV_ProductConfigArray[3] = 0;
}

#define Decipher_Product_Config_1(pos, len) \
    ((NV_ProductConfigArray[1] >> pos) & ((0x00000001 << len) - 1))

DC_Display DeviceConfigClass::getDisplay() {
    return (DC_Display)Decipher_Product_Config_1(DC_Display_Pos, DC_Display_Len);
}

DC_Accel DeviceConfigClass::getAccel() {
    return (DC_Accel)Decipher_Product_Config_1(DC_Accel_Pos, DC_Accel_Len);
}

DC_Enviro DeviceConfigClass::getEnviro() { 
    return (DC_Enviro)Decipher_Product_Config_1(DC_Enviro_Pos, DC_Enviro_Len);
}

DC_Analog DeviceConfigClass::getAnalog() { 
    return (DC_Analog)Decipher_Product_Config_1(DC_Analog_Pos, DC_Analog_Len);
}

DC_Power DeviceConfigClass::getPower() { 
    return (DC_Power)Decipher_Product_Config_1(DC_Power_Pos, DC_Power_Len);
}

// private:
uint32_t DeviceConfigClass::ReverseBitsU32(uint32_t n) {
    n = ((n >> 1) & 0x55555555) | ((n << 1) & 0xaaaaaaaa);
    n = ((n >> 2) & 0x33333333) | ((n << 2) & 0xcccccccc);
    n = ((n >> 4) & 0x0f0f0f0f) | ((n << 4) & 0xf0f0f0f0);
    n = ((n >> 8) & 0x00ff00ff) | ((n << 8) & 0xff00ff00);
    n = ((n >> 16) & 0x0000ffff) | ((n << 16) & 0xffff0000);
    return n;
}
// DeviceConfig.cpp EOF
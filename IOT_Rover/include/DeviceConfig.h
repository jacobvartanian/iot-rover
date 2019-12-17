//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH DeviceConfig.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Filename:	DeviceConfig.h
// Description: Device Configuration
// Author:		Danon Bradford
// Date:		2019-01-02
//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH DeviceConfig.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
#ifndef DeviceConfig_h
#define DeviceConfig_h

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <stdint.h>
#include "ErrorCode.h"

//=============================================================================
// Product Config Code 1
//-----------------------------------------------------------------------------

#define DC_PC1_Default  0x00000249

// Display Configuration
// 00000000 00000000 00000000 00000XXX
#define DC_Display_Pos  0
#define DC_Display_Len  3
typedef enum
{
    DC_Display_Off = 0,
    DC_Display_HT16K33 = 1
} DC_Display;

// Accelerometer Configuration
// 00000000 00000000 00000000 00XXX000
#define DC_Accel_Pos  (DC_Display_Pos + DC_Display_Len)
#define DC_Accel_Len  3
typedef enum
{
    DC_Accel_Off = 0,
    DC_Accel_MMA8452 = 1
} DC_Accel;

// Environment Sensor Configuration
// 00000000 00000000 0000000X XX000000
#define DC_Enviro_Pos  (DC_Accel_Pos + DC_Accel_Len)
#define DC_Enviro_Len  3
typedef enum
{
    DC_Enviro_Off = 0,
    DC_Enviro_DHT11_AM2302_12_2 = 1,
} DC_Enviro;

// Analog Sensor Configuration
// 00000000 00000000 0000XXX0 00000000
#define DC_Analog_Pos  (DC_Enviro_Pos + DC_Enviro_Len)
#define DC_Analog_Len  3
typedef enum
{
    DC_Analog_Off = 0,
    DC_Analog_Mux_Batt_Lux = 1
} DC_Analog;

// Power Mode Configuration
// 00000000 00000000 0XXX0000 00000000
#define DC_Power_Pos  (DC_Analog_Pos + DC_Analog_Len)
#define DC_Power_Len  3
typedef enum
{
    DC_Power_EverythingAlwaysOn = 0,
    DC_Power_UploadThenDeepSleep = 1
} DC_Power;

//=============================================================================
// Class Declaration
//-----------------------------------------------------------------------------
class DeviceConfigClass 
{
    public:
    DeviceConfigClass() {}; // Constructor
    static EC_12S_t SetupError;
    static uint32_t ChipUid;
    static uint32_t ChipPwd;
    static String ChipUidString;
    static String ChipPwdString;
    static char* BlynkTokenNv;
    static bool ValidBlynk;
    static char* PersonNameNv;
    static uint32_t* ProductConfigArrayNv;
    static bool Init();
    static void EraseNv();
    static DC_Display getDisplay();
    static DC_Accel getAccel();
    static DC_Enviro getEnviro();
    static DC_Analog getAnalog();
    static DC_Power getPower();

    private:
    static uint32_t ReverseBitsU32(uint32_t n);
};

//=============================================================================
// Global Instance Declarations (Publicly Accessible)
//-----------------------------------------------------------------------------
extern DeviceConfigClass DeviceConfig;

#endif /* DeviceConfig_h */

// DeviceConfig.h EOF
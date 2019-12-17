//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Sensors.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Filename:	Sensors.h
// Description: Interface with the device sensors of this project.
// Author:		Danon Bradford
// Date:		2018-12-07
//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Sensors.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
#ifndef Sensors_h
#define Sensors_h

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <stdint.h>
#include "ErrorCode.h"
#include "DHTesp.h"
#include "pedometer.h"
#include "Display.h"

//=============================================================================
// Class Declaration
//-----------------------------------------------------------------------------
class SensorsClass 
{
    public:
    SensorsClass() {}; // Constructor
    static uint8_t ShowOnDisplayPrimary;
    static void Init(EC_12S_t *const errorCodePtr);
    static void ShowOnDisplay(uint8_t const show, uint8_t const vpin);

    // DHT11 Sensor
    static uint16_t RawTemperature;
    static uint16_t RawHumidity;
    static uint8_t GetDhtPin();
    static float ConvertTemperature(uint16_t const rawT);
    static float ConvertHumidity(uint16_t const rawH);
    static void SetTemperatureOffset(float const offset);
    static void SetHumidityOffset(float const offset);
    static float GetTemperature();
    static float GetHumidity();
    static float GetHeatIndex();
    static float GetDewPoint();

    // Analog Sensors. Battery and Light
    static uint16_t RawBatteryVoltage;
    static uint16_t RawLightLux;
    static float ConvertBatteryVoltage(uint16_t const rawV);
    static float ConvertLightLux(uint16_t const rawL);
    static float GetBatteryVoltage();
    static float GetLightLux();

    // Accelerometer
    static int AccTID;
    static uint32_t StepCount;
    static uint8_t Orientation;    

    private:
    static DHTesp _Dhtesp;
    static float _tempOffset;
    static float _humOffset;
    static int _DhtTID;
    static void DhtRun();
    static int _AnalogTID;
    static void AnalogRun();
    static TPedometer _Pedometer;
    static void AccRun();
};

//=============================================================================
// Global Instance Declarations (Publicly Accessible)
//-----------------------------------------------------------------------------
extern SensorsClass Sensors;

#endif /* Sensors_h */

// Sensors.h EOF
//////////////////////////////// Sensors.cpp //////////////////////////////////
// Filename:	Sensors.cpp
// Description: Interface with the device sensors of this project.
// Author:		Danon Bradford
// Date:		2018-12-01
//////////////////////////////// Sensors.cpp //////////////////////////////////

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <Arduino.h>
#include <Blynk/BlynkTimer.h>
#include "DeviceConfig.h"
#include "Sensors.h"
#include "VirtualPinDefs.h"

//*****************************************************************************
// Publicly Accessible Global Variable Definitions
//-----------------------------------------------------------------------------
SensorsClass Sensors;

//*****************************************************************************
// Externally Defined Global Variables
//-----------------------------------------------------------------------------
extern BlynkTimer GlobalTimer;

//*****************************************************************************
// Class Member Variable Definitions (static)
//-----------------------------------------------------------------------------
uint8_t SensorsClass::ShowOnDisplayPrimary = 0x00;

// DHT11
// #define DHT11_Debug         1
// #define _DhtPin           2
// #define _DhtRunInerval    1500
DHTesp SensorsClass::_Dhtesp;
DHTesp::DHT_MODEL_t SensorsClass::_DhtModel;
uint8_t SensorsClass::_DhtPin;
uint16_t SensorsClass::_DhtRunInerval;
int SensorsClass::_DhtTID;
uint16_t SensorsClass::RawTemperature;
uint16_t SensorsClass::RawHumidity;

// Analog Sensors 
// #define Analog_Debug        1
#define Analog_SelectPin    14
#define Analog_ReadPin      A0
#define Analog_RunInerval   1000
int SensorsClass::_AnalogTID;
uint16_t SensorsClass::RawBatteryVoltage;
uint16_t SensorsClass::RawLightLux;

// Accelerometer
// #define Acc_Debug           1
#define Acc_RunInerval      300
TPedometer SensorsClass::_Pedometer;
int SensorsClass::AccTID;
uint32_t SensorsClass::StepCount;
uint8_t SensorsClass::Orientation;

// Time of Flight
#define Tof_RunInerval      1000
Distance SensorsClass::_TofSensor;
int SensorsClass::_TofTID;
uint16_t SensorsClass::RawDistance;

//=============================================================================
// Class Member Method Definitions (static)
//-----------------------------------------------------------------------------
void SensorsClass::Init(EC_12S_t *const errorCodePtr) {

    // First prepare if an environment sensor if it is attached
    if (DeviceConfig.getEnviro() == DC_Enviro_DHT11_2) {
        #ifdef DHT11_Debug
            Serial.println("DHT11 on pin 2");
        #endif
        _DhtModel = DHTesp::DHT11;
        _DhtPin = 2;
        _DhtRunInerval = 1500;
    } else if (DeviceConfig.getEnviro() == DC_Enviro_AM2302_12) {
        #ifdef DHT11_Debug
            Serial.println("AM2302 on pin 12");
        #endif
        _DhtModel = DHTesp::AM2302;
        _DhtPin = 12;
        _DhtRunInerval = 2200;
    }
    
    // Force _DhtPin high to end a previous transaction
    if (DeviceConfig.getEnviro() == DC_Enviro_DHT11_2 || DeviceConfig.getEnviro() == DC_Enviro_AM2302_12) {        
        digitalWrite(_DhtPin, HIGH); 
        pinMode(_DhtPin, OUTPUT);
    }

    // Init Analog sensing if it is attached
    if (DeviceConfig.getAnalog() == DC_Analog_Mux_Batt_Lux) {
        // Setup the Analog sensors - battery and light
        pinMode(Analog_SelectPin, OUTPUT);
        _AnalogTID = GlobalTimer.setInterval(Analog_RunInerval, AnalogRun);

        // Run the analog acquisition method once to read the battery voltage
        AnalogRun();
    }    

    // With the battery voltage known, display it on the screen while waiting
    // a period of time.
    // This also helps with the DHT sensor to ensure it has timed out.
    if (DeviceConfig.getDisplay()) {
        if (GetBatteryVoltage() < 1.0f) {
            Display.SetAllPixelsOn();
        } else {
            Display.SetNumber(Display_PRIMARY_Show, GetBatteryVoltage()*10);
            Display.ManualWriteStringStart();
        }
    }
        
    for (uint8_t i = 0; i < 4; i++) {
        delay(500);

        if (DeviceConfig.getDisplay()) {
            Display.Invert();
            Display.WriteBuffer();
        }
    }    

    // Setup the DHT - temperature and humidty sensor
    if (DeviceConfig.getEnviro() == DC_Enviro_DHT11_2 || DeviceConfig.getEnviro() == DC_Enviro_AM2302_12) {
        _Dhtesp.setup(_DhtPin, _DhtModel);
        _DhtTID = GlobalTimer.setInterval(_DhtRunInerval, DhtRun);

        // Log an error if the DHT11 status is not 0
        DhtRun();   // Run the DHT11 sensor to update the status
        ErrorCode_12SLog(errorCodePtr, _Dhtesp.getStatus() != 0);
    }

    // Setup the Accelerometer
    if (DeviceConfig.getAccel() == DC_Accel_MMA8452) {
        byte pedometerSuccess = _Pedometer.Init();
        AccRun();   // Run the accelerometer once, schedule for periodic run
        AccTID = GlobalTimer.setInterval(Acc_RunInerval, AccRun);
    #ifdef Acc_Debug
        Serial.printf("Pedometer initialization: %d\n", pedometerSuccess);
    #endif

        // Log an error if Pedometer.Init() failed
        ErrorCode_12SLog(errorCodePtr, pedometerSuccess == false);
    }

    // Setup the Time of Flight sensor
    byte tofSuccess = false;
    if (_TofSensor.Init()) {
        tofSuccess = true;
        TofRun();   // Run the accelerometer once, schedule for periodic run
        _TofTID = GlobalTimer.setInterval(Tof_RunInerval, TofRun);
    }
    // Log an error if Distance.Init() failed
    ErrorCode_12SLog(errorCodePtr, tofSuccess == false);
}

void SensorsClass::ShowOnDisplay(uint8_t const show, uint8_t const vpin) {
    if (vpin == Temperature_Vpin) {
        Display.SetString(show, String(GetTemperature(),0) + char(176) + "C");
    } else if (vpin == Humidity_Vpin) {
        Display.SetString(show, String(GetHumidity(),0) + char(37));
    } else if (vpin == HeatIndex_Vpin) {
        Display.SetString(show, String(GetHeatIndex(),1) + char(176) + "C");
    } else if (vpin == DewPoint_Vpin) {
        Display.SetString(show, String(GetDewPoint(),1) + char(176) + "C");
    } else if (vpin == BatteryVoltage_Vpin) {
        Display.SetString(show, String(GetBatteryVoltage(),1) + "V");
    } else if (vpin == LightLux_Vpin) {
        Display.SetString(show, String(GetLightLux(),1) + "lx");
    } else if (vpin == StepCount_Vpin){
        Display.SetNumber(show, StepCount);
    } else if (vpin == Orientation_Vpin) {
        Display.SetNumber(show, Orientation);
    } else if (vpin == Distance_Vpin) {
        Display.SetNumber(show, GetDistance());
    }
}

void SensorsClass::DhtRun() {
    uint16_t tempT, tempH;
    tempT = _Dhtesp.getRawTemperature();
    tempH = _Dhtesp.getRawHumidity();    
    if (tempT) RawTemperature = tempT;
    if (tempH) RawHumidity = tempH;

    if (DeviceConfig.getDisplay()) {
        if (ShowOnDisplayPrimary == Temperature_Vpin    ||
            ShowOnDisplayPrimary == Humidity_Vpin       ||
            ShowOnDisplayPrimary == HeatIndex_Vpin      ||
            ShowOnDisplayPrimary == DewPoint_Vpin       ) {
                ShowOnDisplay(Display_PRIMARY_Show, ShowOnDisplayPrimary);
        }
    }

#ifdef DHT11_Debug
    Serial.print("DHT11 Debug");
    Serial.print("\t");
    Serial.print(_Dhtesp.getStatusString());
    Serial.print("\t");
    Serial.print(GetTemperature(), 1);
    Serial.print("\t\t");
    Serial.print(GetHumidity(), 1);
    Serial.print("\t\t");
    Serial.print(GetHeatIndex(), 1);
    Serial.print("\t\t");
    Serial.println(GetDewPoint(), 1);
#endif
}

float SensorsClass::ConvertTemperature(uint16_t const rawT) {
    int16_t convRawTemp;

    if (_DhtModel == DHTesp::DHT11)
        return ((float)rawT / 256.0f) - 4.0f;

    else if (_DhtModel == DHTesp::AM2302) {
        if ( rawT & 0x8000 ) {
            convRawTemp = -(int16_t)(rawT & 0x7FFF);
        } else {
            convRawTemp = (int16_t)rawT;
        }
        return convRawTemp * 0.1;
    }

    else
        return 0;
    
}

float SensorsClass::ConvertHumidity(uint16_t const rawH) {
    if (_DhtModel == DHTesp::DHT11)
        return (float)rawH / 256.0f;

    else if (_DhtModel == DHTesp::AM2302)
        return (float)rawH * 0.1;

    else
        return 0;
}

float SensorsClass::GetTemperature() {
    return ConvertTemperature(RawTemperature);
}

float SensorsClass::GetHumidity() {
    return ConvertHumidity(RawHumidity);
}

float SensorsClass::GetHeatIndex() {
    return _Dhtesp.computeHeatIndex(GetTemperature(), GetHumidity(), false);
}

float SensorsClass::GetDewPoint() {
    return _Dhtesp.computeDewPoint(GetTemperature(), GetHumidity(), false);
}

void SensorsClass::AnalogRun() {
    // Sample battery voltage
    digitalWrite(Analog_SelectPin, LOW);
    delay(2);
    RawBatteryVoltage = (uint16_t)(analogRead(Analog_ReadPin)); // 10-bit result (0-1023), with 1V = 1023

    // Sample light intensity
    digitalWrite(Analog_SelectPin, HIGH);
    delay(2);
    RawLightLux = (uint16_t)(analogRead(Analog_ReadPin)); // 10-bit result (0-1023), with 1V = 1023
    
    if (DeviceConfig.getDisplay()) {
        if (ShowOnDisplayPrimary == BatteryVoltage_Vpin     ||
            ShowOnDisplayPrimary == LightLux_Vpin           ) {
                ShowOnDisplay(Display_PRIMARY_Show, ShowOnDisplayPrimary);
        }
    }

#ifdef Analog_Debug
    Serial.print("Analog Debug");
    Serial.print("\t\t");
    Serial.print(GetBatteryVoltage(), 1);
    Serial.print("\t\t");
    Serial.println(GetLightLux(), 1);
#endif
}

float SensorsClass::ConvertBatteryVoltage(uint16_t const rawV) {
    return (float)rawV / 1023.0f * (75.0f + 390.0f) / 75.0f;
}

float SensorsClass::ConvertLightLux(uint16_t const rawL) {
    if (rawL < 91) {
        return 0.0f;
    } else {
        // O4 = analogRead of light sensor. O4 is the cell in the excel sheet.
        #define O4 ((float)rawL)
        return ( (O4*O4*0.0008f)+(O4*1.4563f)-138.9f );
    }
}

float SensorsClass::GetBatteryVoltage() {
    return ConvertBatteryVoltage(RawBatteryVoltage);
}

float SensorsClass::GetLightLux() {
    return ConvertLightLux(RawLightLux);
}

uint16_t SensorsClass::GetDistance() {
    return RawDistance;
}

void SensorsClass::AccRun() {    
    // Update pedometer
    _Pedometer.Update();
    StepCount = _Pedometer.StepCount;
    Orientation = _Pedometer.Rotation;

    if (DeviceConfig.getDisplay()) {
        Display.UpdateRotation(Orientation);
        if (ShowOnDisplayPrimary == StepCount_Vpin      ||
            ShowOnDisplayPrimary == Orientation_Vpin    ) {
                ShowOnDisplay(Display_PRIMARY_Show, ShowOnDisplayPrimary);
        }
    }

#ifdef Acc_Debug
    if (_pedometer.RotationHasChanged || _pedometer.StepCountHasChanged) {
        Serial.print("Acc Debug");
        Serial.print("\t\t");
        Serial.print(StepCount);
        Serial.print("\t\t");
        Serial.println(Orientation);
    }
#endif
}

void SensorsClass::TofRun() {
    // Update distance reading
    RawDistance = _TofSensor.GetDistance();
    if (DeviceConfig.getDisplay()) {
        if (ShowOnDisplayPrimary == Distance_Vpin) {
            ShowOnDisplay(Display_PRIMARY_Show, ShowOnDisplayPrimary);
        }
    }
}
// Sensors.cpp EOF
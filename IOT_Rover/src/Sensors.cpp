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
#define DhtPin_V11 2u   // Hardware pin for DHT11, on PCB V1.1
#define DhtPin_V13 12u  // Hardware pin for DHT11 and DHT22|AM2302, on PCB V1.2 and V1.3
DHTesp SensorsClass::_Dhtesp;
int SensorsClass::_DhtTID = -1;
float SensorsClass::_tempOffset = 0;
float SensorsClass::_humOffset = 0;
uint16_t SensorsClass::RawTemperature;
uint16_t SensorsClass::RawHumidity;

// Analog Sensors 
// #define Analog_Debug        1
#define Analog_SelectPin    14u
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

//=============================================================================
// Class Member Method Definitions (static)
//-----------------------------------------------------------------------------
void SensorsClass::Init(EC_12S_t *const errorCodePtr) {

    uint16_t runInerval = 0u;
    
    // Force DhtPin pins high to end a previous transaction.
    // This effectively timeouts the sensor.
    if (DeviceConfig.getEnviro() == DC_Enviro_DHT11_AM2302_12_2) {        
        digitalWrite(DhtPin_V11, HIGH); 
        pinMode(DhtPin_V11, OUTPUT);
        digitalWrite(DhtPin_V13, HIGH); 
        pinMode(DhtPin_V13, OUTPUT);
    }

    // Init Analog sensing if it is attached
    if (DeviceConfig.getAnalog() == DC_Analog_Mux_Batt_Lux) {
        // Setup the Analog sensors - battery and light
        pinMode(Analog_SelectPin, OUTPUT);
        _AnalogTID = GlobalTimer.setInterval(Analog_RunInerval, AnalogRun);

        // Run the analog acquisition method once to read the battery voltage
        AnalogRun();
    }    

    // With the battery voltage known, display it on the screen.
    if (DeviceConfig.getDisplay()) {
        if (GetBatteryVoltage() < 1.0f) {
            Display.SetAllPixelsOn();
        } else {
            Display.SetNumber(Display_PRIMARY_Show, GetBatteryVoltage()*10);
            Display.ManualWriteStringStart();
        }
    }

    // Waiting a period of time, do things within the wait period.
    for (uint8_t i = 0; i < 4; i++) {
        delay(500);

        // Invert the battery voltage on the display.
        if (DeviceConfig.getDisplay()) {
            Display.Invert();
            Display.WriteBuffer();
        }

        // Begin detecting the DHT sensor. 
        // This wait loop is important. At i==1 we have waited 1 second 
        // for the previous DhtPin pin setting to timeout the DHTs.
        // We can now try and auto detect the DHTs and then wait another second!
        if (i == 1 && DeviceConfig.getEnviro() == DC_Enviro_DHT11_AM2302_12_2) {
            // First try and detect a DHT sensor on the V1.3 hardware pin.
            _Dhtesp.setup(DhtPin_V13, DHTesp::AUTO_DETECT);
        }
    }   

    // Setup the DHT - temperature and humidty sensor
    if (DeviceConfig.getEnviro() == DC_Enviro_DHT11_AM2302_12_2) {      

        // Force the sensor menthod to run to see if it was detected
        // on the V1.3 hardware pin.
        DhtRun(); 

        if (_Dhtesp.getStatus() == DHTesp::ERROR_NONE) {
            // Fantastic! The sensor is attached to DhtPin_V13.

            // Different sensors have a different minimum polling period
            if (_Dhtesp.getModel() == DHTesp::DHT22 || _Dhtesp.getModel() == DHTesp::AM2302)
                runInerval = 2200u;

            else if (_Dhtesp.getModel() == DHTesp::DHT11)
                runInerval = 1500u;

            ErrorCode_12SLog(errorCodePtr, ErrorCode_PASS);
        }

        else {
            // Try to detect the DHT11 on the V1.1 hardware pin
            _Dhtesp.setup(DhtPin_V11, DHTesp::DHT11);

            // Force the sensor menthod to run to see if it was detected
            DhtRun(); 

            if (_Dhtesp.getStatus() == DHTesp::ERROR_NONE) {
                // Fantastic! The sensor is attached to DhtPin_V11.
                runInerval = 1500u;
                ErrorCode_12SLog(errorCodePtr, ErrorCode_PASS);
            }

            else
                ErrorCode_12SLog(errorCodePtr, ErrorCode_FAIL);            
        }

        // Schedule the DHT to periodically run if a valid sensor was detected
        if (runInerval)
            _DhtTID = GlobalTimer.setInterval(runInerval, DhtRun);        
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
    Serial.print("DHT Debug");
    Serial.print("\t");
    Serial.print(_Dhtesp.getStatusString());
    Serial.print("\t");
    Serial.print(_Dhtesp.getModel());
    Serial.print("\t");
    Serial.print(_Dhtesp.getPin());
    Serial.print("\t");
    Serial.print(tempT);
    Serial.print("\t\t");
    Serial.print(tempH);
    Serial.print("\t\t");
    Serial.print(GetTemperature(), 1);
    Serial.print("\t\t");
    Serial.print(GetHumidity(), 1);
    Serial.print("\t\t");
    Serial.print(GetHeatIndex(), 1);
    Serial.print("\t\t");
    Serial.println(GetDewPoint(), 1);
#endif
}

uint8_t SensorsClass::GetDhtPin() {
    if (_DhtTID != -1)
        return _Dhtesp.getPin();
    else
        return 0;    
}

float SensorsClass::ConvertTemperature(uint16_t const rawT) {
    float convRawTemp = 0;

    if (_Dhtesp.getModel() == DHTesp::DHT11) {
        convRawTemp = (rawT >> 8) + ((rawT & 0x00FF) * 0.1);
    }

    else if (_Dhtesp.getModel() == DHTesp::DHT22 || _Dhtesp.getModel() == DHTesp::AM2302) {
        if ( rawT & 0x8000 ) {
            convRawTemp = -(int16_t)(rawT & 0x7FFF);
        } else {
            convRawTemp = (int16_t)rawT;
        }
        convRawTemp *= 0.1;
    }

    return convRawTemp + _tempOffset;    
}

float SensorsClass::ConvertHumidity(uint16_t const rawH) {
    float convRawHum = 0;

    if (_Dhtesp.getModel() == DHTesp::DHT11)
        convRawHum = (rawH >> 8) + ((rawH & 0x00FF) * 0.1);

    else if (_Dhtesp.getModel() == DHTesp::DHT22 || _Dhtesp.getModel() == DHTesp::AM2302)
        convRawHum = (float)rawH * 0.1;

    return convRawHum + _humOffset;
}

void SensorsClass::SetTemperatureOffset(float const offset) {
    _tempOffset = offset;
}

void SensorsClass::SetHumidityOffset(float const offset) {
    _humOffset = offset;
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

// Sensors.cpp EOF
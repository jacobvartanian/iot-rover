//////////////////////////////// Mobility.cpp /////////////////////////////////
// Filename:	Mobility.cpp
// Description: Top level module that controls moving the IDL around.
// Author:		Danon Bradford
// Date:		2019-12-07
//////////////////////////////// Mobility.cpp /////////////////////////////////

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
// ----------------------------------------------------------------------------
#include "Mobility.h"  // Source Header file
#include <Arduino.h>   // Arduino Header file
#include <Wire.h>      // I2C Header file

//*****************************************************************************
// Private Macro Definitions
//-----------------------------------------------------------------------------
#define MaxSteer 10
#define SpeedDivisor (MaxSteer / 2)

//*****************************************************************************
// Publicly Accessible Global Variable Definitions
//-----------------------------------------------------------------------------
MobilityClass Mobility{DRV8830_Addr0, DRV8830_Addr2};

//=============================================================================
// Class Member Method Definitions
//-----------------------------------------------------------------------------
// public:
bool MobilityClass::SetDrive(int8_t const speed, int8_t const steer) {
    int8_t majorSpeed = speed;
    int8_t minorSpeed = 0;

    uint8_t speedAbs = 0;
    uint8_t steerAbs = 0;
    uint8_t subMul = 0;
    uint8_t subAmmount = 0;

    if (majorSpeed == 0) {
        this->_Motors[Mobility_FrontLeftMotor].SetCoast();
        this->_Motors[Mobility_FrontRightMotor].SetCoast();
    } else {
        steerAbs = abs(steer);

        if (steerAbs == 0) {
            minorSpeed = majorSpeed;
        } else if (steerAbs == 5) {
            minorSpeed = 0;
        } else if (steerAbs == 10) {
            minorSpeed = majorSpeed * -1;
        } else {
            speedAbs = abs(majorSpeed);
            subMul = speedAbs / SpeedDivisor;
            subAmmount = steerAbs * subMul;
            minorSpeed = speedAbs - subAmmount;

            if (majorSpeed < 0) minorSpeed *= -1;
        }

        if (steer >= 0) {
            this->_Motors[Mobility_FrontLeftMotor].SetSpeedDir(majorSpeed);
            this->_Motors[Mobility_FrontRightMotor].SetSpeedDir(minorSpeed);
        } else {
            this->_Motors[Mobility_FrontRightMotor].SetSpeedDir(majorSpeed);
            this->_Motors[Mobility_FrontLeftMotor].SetSpeedDir(minorSpeed);
        }
    }

    bool success[4] = {
        this->_Motors[Mobility_FrontLeftMotor].ClearFaultReg(),
        this->_Motors[Mobility_FrontLeftMotor].WriteControlReg(),
        this->_Motors[Mobility_FrontRightMotor].ClearFaultReg(),
        this->_Motors[Mobility_FrontRightMotor].WriteControlReg()};

    return (success[0] && success[1] && success[2] && success[3]);
}

void MobilityClass::PrintMotorFaults() {
    uint8_t fault = 0;
    this->_Motors[Mobility_FrontLeftMotor].GetFaultReg(&fault);
    Serial.println(fault, HEX);
    fault = 0;
    this->_Motors[Mobility_FrontRightMotor].GetFaultReg(&fault);
    Serial.println(fault, HEX);
}

// Mobility.cpp EOF
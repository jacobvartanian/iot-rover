//////////////////////////////// DRV8830.cpp //////////////////////////////////
// Filename:	DRV8830.cpp
// Description: Low-Voltage Motor Driver With I2C Interface.
// Author:		Danon Bradford
// Date:		2019-12-05
//////////////////////////////// DRV8830.cpp //////////////////////////////////

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
// ----------------------------------------------------------------------------
#include "DRV8830.h"  // Source Header file
#include <Arduino.h>  // Arduino Header file
#include <Wire.h>     // I2C Header file

//*****************************************************************************
// Private Macro Definitions
//-----------------------------------------------------------------------------
#define VsetMax 0x3Fu               // 63
#define VsetMin 0x06u               //  6
#define MaxSpeed VsetMax - VsetMin  // 57
// #define DRV8830_Debug 1

//=============================================================================
// Class Member Method Definitions
//-----------------------------------------------------------------------------
// private:
bool DRV8830Class::WriteRegByte(DRV8830_Register const reg,
                                uint8_t const byte) {
#ifdef DRV8830_Debug
    Serial.print(String("DRV8830Class::WriteRegByte(" + String(reg, HEX) +
                        ", " + String(byte, HEX) + ") = "));
#endif
    Wire.beginTransmission(this->_I2CAddr);

    if (Wire.write(reg) == 1 && Wire.write(byte) == 1 &&
        Wire.endTransmission(true) == 0) {
#ifdef DRV8830_Debug
        Serial.println("true");
#endif
        return true;
    }
#ifdef DRV8830_Debug
    Serial.println("false");
#endif
    return false;
}

// public:
bool DRV8830Class::GetFaultReg(uint8_t *const dataPtr) {
#ifdef DRV8830_Debug
    Serial.print(String("DRV8830Class::GetFaultReg(" +
                        String((uint32_t)dataPtr, HEX) + ") = "));
#endif
    Wire.beginTransmission(this->_I2CAddr);

    if (Wire.write(DRV8830_Fault) == 1 && Wire.endTransmission(0x00u) == 0 &&
        Wire.requestFrom(this->_I2CAddr, 0x01u, 0x01u) == 1) {
        if (dataPtr) *dataPtr = Wire.read();
#ifdef DRV8830_Debug
        Serial.println("true, " + String(*dataPtr, HEX));
#endif
        return true;
    }
#ifdef DRV8830_Debug
    Serial.println("false, " + String(*dataPtr, HEX));
#endif
    return false;
}

bool DRV8830Class::ClearFaultReg() { return WriteRegByte(DRV8830_Fault, 0x80); }

// The speed (VSET) and bridge control share the same register
//  D7 - D2    D1    D0
// VSET[5..0]  IN2   IN1

void DRV8830Class::SetBridgeControl(DRV8830_BridgeLogic const data) {
    this->_ControlReg = (this->_ControlReg & 0xFCu) | data;
}

// Set Voltage Control
void DRV8830Class::SetVoltage(uint8_t const speed) {
    uint8_t vset = (speed > MaxSpeed ? MaxSpeed : speed) + VsetMin;
    this->_ControlReg = (vset << 2) | (this->_ControlReg & 0x03u);
}

void DRV8830Class::SetSpeedDir(int8_t const speed) {
    uint8_t vset = (abs(speed) > MaxSpeed ? MaxSpeed : abs(speed)) + VsetMin;
    uint8_t bridge =
        (speed == 0 ? DRV8830_Coast
                    : (speed > 0 ? DRV8830_Forward : DRV8830_Reverse));
    this->_ControlReg = (vset << 2) | bridge;
}

void DRV8830Class::SetCoast() { this->_ControlReg = 0x00u; }

bool DRV8830Class::WriteControlReg() {
    return WriteRegByte(DRV8830_Control, this->_ControlReg);
}

// DRV8830.cpp EOF
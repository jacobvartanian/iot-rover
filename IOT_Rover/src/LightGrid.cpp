//////////////////////////////// LightGrid.cpp ////////////////////////////////
// Filename:	LightGrid.cpp
// Description: Control the pixels of a HT16K33 LED matrix light grid.
// Author:		Danon Bradford
// Date:		2018-11-13
//////////////////////////////// LightGrid.cpp ////////////////////////////////

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <Arduino.h>				// Arduino Header file
#include <Wire.h>					// I2C Header file
#include <stdint.h>					// Standard Integer Header file
#include "LightGrid.h"				// Source Header file

//*****************************************************************************
// Private Macro Definitions
//-----------------------------------------------------------------------------
#define SystemSetupCMD		0x20
#define RowIntCMD			0xA0
#define DisplayCMD			0x80
#define DimmingCMD			0xE0
#define RamStartCMD			0x00

//*****************************************************************************
// Class Member Variable Definitions (static)
//-----------------------------------------------------------------------------
// private:
uint8_t const LightGrid::_AddressI2C = 0x70;

// Hardcode lookup table to know how the grid is wired.
uint8_t const LightGrid::_RowLookup[12] = {7,6,3,2,0,1,4,5,8,11,10,9};
uint8_t const LightGrid::_ColumnLookup[8] = {2,1,3,4,5,6,7,0};

// The ht16K33 ram structure is grouped by 8 columns.
// Each bit in a u16 represents a pixel in that column.
// Each bit is effectively a row.
uint16_t LightGrid::_Buffer[8] = {0x0000};

//=============================================================================
// Class Member Method Definitions (static)
//-----------------------------------------------------------------------------
// public:
void LightGrid::Init(void) {
    // When power is applied, System Oscillator will be in an off state.
    // Data transfers on the I2C-bus should be avoided for 1 ms following 
    // a power-on to allow completion of the reset action.
    // The device is now in standby mode (System Oscillator Off).
    delay(1);

    // Set the Row/Int pin to a Row driver output.
    // Should be 0 on reset and never changed.
    // if (WriteByte(RowIntCMD | 0x00) == false)
    // 	return false;

    // Nothing to do!
}

bool LightGrid::Power(uint8_t const onOff) {
    // return SystemOscillatorOnOff(onOff);
    return WriteByte(SystemSetupCMD | (0x01 & onOff));
}

bool LightGrid::Display(uint8_t const onOff) {
    return WriteByte(DisplayCMD | (0x01 & onOff));
}

bool LightGrid::Blink(uint8_t const blink) {
    return WriteByte(DisplayCMD | ((0x03 & blink) << 1));
}

bool LightGrid::Brightness(uint8_t const bright) {
    return WriteByte(DimmingCMD | (0x0F & bright));
}

void LightGrid::SetPixel(uint8_t const x, uint8_t const y, uint8_t const onOff) {	
    if (onOff)
        _Buffer[_ColumnLookup[x]] |= 0x01 << y;

    else
        _Buffer[_ColumnLookup[x]] &= ~(0x01 << y);
}

void LightGrid::SetColumn(uint8_t const column, uint16_t const val) {
    // Place the row data into the correct column that matches the board wiring.
    _Buffer[_ColumnLookup[column]] = val;
}

void LightGrid::SetRow(uint8_t const row, uint8_t const val) {
    for (uint8_t col = 0; col < 8; col++) {
        if (val & (0x01 << col)) 
            _Buffer[_ColumnLookup[col]] |= 0x01 << row;

        else
            _Buffer[_ColumnLookup[col]] &= ~(0x01 << row);
    }
}

bool LightGrid::WriteBuffer(void) {
    Wire.beginTransmission(_AddressI2C);

    if (Wire.write(RamStartCMD) != 1)
        return false;

    // Write out all 8 columns, half at a time (8 bits)
    for (uint8_t col = 0; col < 8; col++) {	
        uint16_t correctRowData = 0x0000u;

        for (uint8_t row = 0; row < 12; row++) {
            if (_Buffer[col] & (0x0001u << row))
                correctRowData |= 0x0001 << _RowLookup[row];
        }

        if (Wire.write((uint8_t)(correctRowData >> 0)) == 0)
            return false;

        if (Wire.write((uint8_t)(correctRowData >> 8)) == 0)
            return false;
    }

    if (Wire.endTransmission() != 0)
          return false;
    
      return true;
}

void LightGrid::ClearBuffer(void) {
    for (uint8_t col = 0; col < 8; col++) {
        _Buffer[col] = 0x0000;
    }
}

void LightGrid::InvertBuffer(void) {
    for (uint8_t col = 0; col < 8; col++) {
        _Buffer[col] = ~_Buffer[col];
    }
}

// private:
bool LightGrid::WriteByte(uint8_t const byte) {
    Wire.beginTransmission(_AddressI2C);
    
    if (Wire.write(byte) == 1) {
          if (Wire.endTransmission() == 0)
              return true;
    }

    return false;
}

// LightGrid.c EOF
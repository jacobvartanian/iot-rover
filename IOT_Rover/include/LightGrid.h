//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH LightGrid.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Filename:	LightGrid.h
// Description: Control the pixels of a HT16K33 LED matrix light grid.
// Author:		Danon Bradford
// Date:		2018-11-13
//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH LightGrid.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH

#ifndef LightGrid_h
#define LightGrid_h

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <stdint.h>					// Standard Integer Header file

//=============================================================================
// Class Declaration
//-----------------------------------------------------------------------------
class LightGrid {
    
    public:
    LightGrid() {} // Constructor
    static void Init();
    static bool Power(uint8_t const onOff);
    static bool Display(uint8_t const onOff);
    static bool Blink(uint8_t const blink);
    static bool Brightness(uint8_t const bright);
    static void SetPixel(uint8_t const x, uint8_t const y, uint8_t const onOff);
    static void SetColumn(uint8_t const column, uint16_t const val);
    static void SetRow(uint8_t const row, uint8_t const val);
    static bool WriteBuffer();
    static void ClearBuffer();
    static void InvertBuffer();

    private:
    static uint8_t const _AddressI2C;
    static uint8_t const _RowLookup[12];
    static uint8_t const _ColumnLookup[8];
    static uint16_t _Buffer[8];
    static bool WriteByte(uint8_t const byte);
};

#endif /* LightGrid_h */

// LightGrid.h EOF
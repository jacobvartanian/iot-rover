//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Display.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Filename:	Display.h
// Description: Control the IDL display.
// Author:		Danon Bradford
// Date:		2018-12-01
//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Display.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH

#ifndef Display_h
#define Display_h

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <stdint.h>   
#include "LightGrid.h"	

//=============================================================================
// Public Macro Definitions
//-----------------------------------------------------------------------------
#define Display_PowerOff_Mode       0
#define Display_DisplayOff_Mode     1
#define Display_AllLedsOff_Mode     2
#define Display_AllLedsOn_Mode      3
#define Display_String_Mode         4
#define Display_U64_Mode            5
#define Display_Manual_Mode         6

#define Display_PRIMARY_Show 	    0
#define Display_TEMPORARY_Show 	    1

//=============================================================================
// Class Declaration
//-----------------------------------------------------------------------------
class DisplayClass {
    public:
    DisplayClass() {} // Constructor
    static bool Init();
    static void UpdateRotation(uint8_t const rotation);
    static void SetMode(uint8_t const show, uint8_t const mode);
    static void SetString(uint8_t const show, String textString);
    static void SetNumber(uint8_t const show, int const number);
    static void SetU64Image(uint8_t const show, uint64_t const image, uint8_t const index);
    static void SetU64Count(uint8_t const show, uint8_t const count);
    static void SetBrightness(const uint8_t bright);
    static void PixelsOnOff(const uint8_t onOff);
    static void ActivateTempShow(uint32_t msTime);
    static void ScrollReset();
    static void SetScrollInterval(uint32_t msTime);
    static void ScrollEnable(bool const onOff);
    static void Clear();
    static void Invert();
    static void SetPixel(uint8_t const x, uint8_t const y, uint8_t const onOff);
    static void SetAllPixelsOn();
    static void ManualWriteStringStart();
    static void WriteBuffer();

    private:
    static LightGrid _LightGrid;    
    static int _DispUpdaTID;
    static int _TempStrTID;
    static uint8_t _Mode[2];    // The PRIMARY and TEMPORARY modes. 
    static uint8_t _Show;       // Showing either PRIMARY or TEMPORARY.
    static uint8_t _Rotation;
    static uint8_t _StringCharArray[2][50];
    static uint8_t _StringLength[2];
    static uint64_t _U64ImageArray[2][9];
    static uint8_t _U64ImageCount[2];
    static uint8_t _ScrollElement;
    static uint8_t _ScrollWindow;
    static void LoadPartOfString(uint8_t const element, uint8_t const window);
    static void CheckActIfScrollEnabled();
    static void ScrollString();
    static void TempShowTimeout();
    static void SetTrippleImage64(uint64_t const aa64, uint64_t const bb64, uint64_t const cc64, uint8_t leftShift);
};

//=============================================================================
// Global Instance Declarations (Publicly Accessible)
//-----------------------------------------------------------------------------
extern DisplayClass Display;

#endif /* Display_h */

// Display.h EOF
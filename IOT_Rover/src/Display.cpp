//////////////////////////////// Display.cpp //////////////////////////////////
// Filename:	Display.cpp
// Description: Control the IDL display.
// Author:		Danon Bradford
// Date:		2018-12-01
//////////////////////////////// Display.cpp //////////////////////////////////

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <Arduino.h>				// Arduino Header file
#include <Blynk/BlynkTimer.h>   
#include "AsciiArray.h"				// Ascii Array Header file
#include "Display.h"				// Source Header file

//*****************************************************************************
// Publicly Accessible Global Variable Definitions
//-----------------------------------------------------------------------------
DisplayClass Display;

//*****************************************************************************
// Externally Defined Global Variables
//-----------------------------------------------------------------------------
extern BlynkTimer GlobalTimer;

//*****************************************************************************
// Private Global Variables
//-----------------------------------------------------------------------------
// Fastest way to reverse the bits in a U8
static const uint8_t ReverseU8[] = {
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

//*****************************************************************************
// Class Member Variable Definitions (static)
//-----------------------------------------------------------------------------
int DisplayClass::_DispUpdaTID = -1;
int DisplayClass::_TempStrTID = -1;
uint8_t DisplayClass::_Mode[2] = {Display_String_Mode, Display_String_Mode};
uint8_t DisplayClass::_Show = Display_PRIMARY_Show;
uint8_t DisplayClass::_Rotation = 0x03;

// Buffer for scrolling text modes.
uint8_t DisplayClass::_StringCharArray[2][50] = {32};
uint8_t DisplayClass::_StringLength[2] = {0};
uint64_t DisplayClass::_U64ImageArray[2][9] = {0};
uint8_t DisplayClass::_U64ImageCount[2] = {0};

// Scrolling text variables.
uint8_t DisplayClass::_ScrollElement = 0;
uint8_t DisplayClass::_ScrollWindow = 0;

//=============================================================================
// Class Member Method Definitions (static)
//-----------------------------------------------------------------------------
// public:
bool DisplayClass::Init(void) {
    _LightGrid.Init();

    // Periodically scroll a string on the display.
    _DispUpdaTID = GlobalTimer.setInterval(50L, ScrollString);

    // Power on the Light Grid, 
    // Turn on the display and
    // Set the brightness to the highest level.
    // Return true if all this worked!
    return (
        _LightGrid.Power(1) 			&&
        _LightGrid.Display(1) 			&&
        _LightGrid.Brightness(7)	
    );
}

void DisplayClass::UpdateRotation(uint8_t const rotation) {
    static uint8_t prevRotation = 0x03;

    if (!(rotation&0x40)) {
        _Rotation = rotation & 0x03;

        if (_Rotation != prevRotation) {
            prevRotation = _Rotation;
            CheckActIfScrollEnabled();
        }
    }	
}
void DisplayClass::SetMode(uint8_t const show, uint8_t const mode) {
    _Mode[show] = mode;

    if (show == Display_PRIMARY_Show) {

        if (mode == Display_PowerOff_Mode) {
            _LightGrid.Power(0);
        } else {
            _LightGrid.Power(1);
        }

        if (mode == Display_DisplayOff_Mode) {
            _LightGrid.Display(0);
        } else {
            _LightGrid.Display(1);
        }

        if (mode == Display_AllLedsOff_Mode) {
            Clear();
            WriteBuffer();
        } else if (mode == Display_AllLedsOn_Mode) {
            SetAllPixelsOn();            
        }
    }
}

void DisplayClass::SetString(uint8_t const show, String textString) {
    _StringLength[show] = textString.length();

    textString.toCharArray((char*)&_StringCharArray[show][2], 45);

    // Make sure the first characters are blank
    _StringCharArray[show][0] = 32;
    _StringCharArray[show][1] = 32;

    // Make sure the end characters are blank
    _StringCharArray[show][2 + _StringLength[show] + 0] = 32;
    _StringCharArray[show][2 + _StringLength[show] + 1] = 32;
    _StringCharArray[show][2 + _StringLength[show] + 2] = 32;

    CheckActIfScrollEnabled();
}

void DisplayClass::SetNumber(uint8_t const show, int const number) {
    SetString(show, String(number,DEC));
}

void DisplayClass::SetU64Image(uint8_t const show, uint64_t const image, uint8_t const index) {
    _U64ImageArray[show][index+2] = image;	
}

void DisplayClass::SetU64Count(uint8_t const show, uint8_t const count) {
    _U64ImageCount[show] = count;
    CheckActIfScrollEnabled();
}

void DisplayClass::SetBrightness(const uint8_t bright) {
    _LightGrid.Brightness(bright);
}

void DisplayClass::PixelsOnOff(const uint8_t onOff) {
    _LightGrid.Display(onOff);
}

void DisplayClass::ActivateTempShow(uint32_t msTime) {

    if (!msTime) return;
    _Show = Display_TEMPORARY_Show;

    // Delete the timer if it's already running.
    if (_TempStrTID != -1)
        GlobalTimer.deleteTimer(_TempStrTID);

    _TempStrTID = GlobalTimer.setTimeout(msTime, TempShowTimeout);
    CheckActIfScrollEnabled();
}

void DisplayClass::ScrollReset() {	
    _ScrollWindow = 0;
    _ScrollElement = 0;
}

void DisplayClass::SetScrollInterval(uint32_t msTime) {
    (void)GlobalTimer.changeInterval(_DispUpdaTID, msTime);
}

void DisplayClass::ScrollEnable(bool const onOff) {	
    if (onOff) {
        _ScrollWindow = 0;
        _ScrollElement = 0;
        GlobalTimer.enable(_DispUpdaTID);
    } else {
        GlobalTimer.disable(_DispUpdaTID);
        ManualWriteStringStart();
    }
}

void DisplayClass::Clear() {
    _LightGrid.ClearBuffer();
}

void DisplayClass::Invert() {
    _LightGrid.InvertBuffer();
}

void DisplayClass::SetPixel(uint8_t const x, uint8_t const y, uint8_t const onOff) {
    return _LightGrid.SetPixel(x, y, onOff);
}

void DisplayClass::SetAllPixelsOn() {
    for (uint8_t count = 0; count < 8; count++) {
        _LightGrid.SetColumn(count, 0xFFFF);
    }
    WriteBuffer();
}

void DisplayClass::ManualWriteStringStart() {
    if (_Mode[_Show] != Display_String_Mode && _Mode[_Show] != Display_U64_Mode) return;

    uint8_t stringLength = _Mode[_Show] == Display_U64_Mode ? _U64ImageCount[_Show] : _StringLength[_Show];

    if (stringLength == 1)
        LoadPartOfString(1,6);
    else
        LoadPartOfString(2,0);

    _LightGrid.WriteBuffer();
}

void DisplayClass::WriteBuffer() {
    _LightGrid.WriteBuffer();
}

// private:
void DisplayClass::LoadPartOfString(uint8_t const element, uint8_t const window) {
    
    if (_Mode[_Show] == Display_U64_Mode) {
        uint64_t *arrayPtr = _U64ImageArray[_Show];
        SetTrippleImage64(arrayPtr[element+0], arrayPtr[element+1], arrayPtr[element+2], window);
    } else {
        uint64_t aa = AsciiArray[_StringCharArray[_Show][element+0]];
        uint64_t bb = AsciiArray[_StringCharArray[_Show][element+1]];
        uint64_t cc = AsciiArray[_StringCharArray[_Show][element+2]];
        // String hexString = String((uint32_t)(aa>>32) ,HEX) + String((uint32_t)aa ,HEX);
        // while (hexString.length() < 16) {
        //     hexString = String("0") + hexString;
        // }
        // Serial.println(String("aa = ") + hexString);
        SetTrippleImage64(aa, bb, cc, window);
    }
}

void DisplayClass::CheckActIfScrollEnabled() {
    if (!GlobalTimer.isEnabled(_DispUpdaTID))	
        ManualWriteStringStart();
}

void DisplayClass::ScrollString() {

    if (_Mode[_Show] != Display_String_Mode && _Mode[_Show] != Display_U64_Mode) return;
    
    LoadPartOfString(_ScrollElement, _ScrollWindow++);	
    
    uint8_t stringLength = _Mode[_Show] == Display_U64_Mode ? _U64ImageCount[_Show] : _StringLength[_Show];
    if (_ScrollWindow > 8) {
        _ScrollWindow = 1;
        _ScrollElement++;

        if (_ScrollElement > stringLength + 1)
            _ScrollElement = 0;
    }

    _LightGrid.WriteBuffer();
}

void DisplayClass::TempShowTimeout() {
    _TempStrTID = -1;
    _Show = Display_PRIMARY_Show;

    if (_Mode[_Show] == Display_AllLedsOff_Mode || _Mode[_Show] == Display_Manual_Mode ) {
        Clear();
        WriteBuffer();
    } else if (_Mode[_Show] == Display_AllLedsOn_Mode) {
        SetAllPixelsOn();            
    }

    CheckActIfScrollEnabled();
}

// Each image is 64 bits of data.
// uint64_t Image = 0x0123456789ABCDEF;
// 0x01 = Bottom row of image. 1 is the left most LED's, 0 is the right most LED's.
// In other words, this is the image
// FFFFEEEE
// DDDDCCCC
// BBBBAAAA
// 99998888
// 77776666
// 55554444
// 33332222
// 11110000

// void SetSingleImage64_NoRotation(uint64_t const image64, uint8_t const leftShift)
// {
// 	for (uint8_t imageRow = 0; imageRow < 8; imageRow++)
// 	{
// 		uint8_t rowData = image64 >> (imageRow*8);
// 		rowData = ReverseU8[rowData];
// 		LightGrid.SetColumn(7-imageRow, (uint16_t)rowData << leftShift);
// 	}
// }

// Display an image like this
//             |------------|					leftShift
//     00000000|AAAAAAAABBBB|BBBBCCCCCCCC		0
//     0000000A|AAAAAAABBBBB|BBBCCCCCCCC0		1
//           AA|AAAAAABBBBBB|BBCCCCCCCC			2
//          AAA|AAAAABBBBBBB|BCCCCCCCC			3
//         AAAA|AAAABBBBBBBB|CCCCCCCC			4
//        AAAAA|AAABBBBBBBBC|CCCCCCC			5
//       AAAAAA|AABBBBBBBBCC|CCCCCC				6
//      AAAAAAA|ABBBBBBBBCCC|CCCCC				7
//     AAAAAAAA|BBBBBBBBCCCC|CCCC				Same as 0!
void DisplayClass::SetTrippleImage64(uint64_t const aa64, uint64_t const bb64, uint64_t const cc64, uint8_t leftShift) {
    uint8_t rowArray[24];

    if (leftShift > 12)
        leftShift = 12;
    
    for (uint8_t imageRow = 0; imageRow < 8; imageRow++) {
        uint8_t rowDataA = aa64 >> (imageRow*8);
        uint8_t rowDataB = bb64 >> (imageRow*8);
        uint8_t rowDataC = cc64 >> (imageRow*8);

        // Upside down?
        if (_Rotation == 0x03 || _Rotation == 0x00)	{
            rowDataA = ReverseU8[rowDataA];
            rowDataB = ReverseU8[rowDataB];
            rowDataC = ReverseU8[rowDataC];
        }
        
        // Display Landscape Mode = 12 Wide by 8 Tall Pixels
        if (_Rotation & 0x02) {
            uint32_t rowData32;
            
            if (_Rotation & 0x01) {				
                rowData32 = ((((uint32_t)rowDataA) << 16) & 0x00FF0000) | ((((uint32_t)rowDataB) << 8) & 0x0000FF00) | ((((uint32_t)rowDataC) << 0) & 0x000000FF);
                _LightGrid.SetColumn(7-imageRow, (uint16_t)(rowData32 >> (12 - leftShift)));
            } else {
                rowData32 = ((((uint32_t)rowDataC) << 16) & 0x00FF0000) | ((((uint32_t)rowDataB) << 8) & 0x0000FF00) | ((((uint32_t)rowDataA) << 0) & 0x000000FF);
                _LightGrid.SetColumn(imageRow, (uint16_t)(rowData32 >> (leftShift)));
            }	
        }

        // Display Portrait Mode = 8 Wide by 12 Tall Pixels
        else {
            rowArray[23-imageRow] = rowDataA;
            rowArray[15-imageRow] = rowDataB;
            rowArray[7-imageRow] = rowDataC;
        }
    }

    // Display Portrait Mode = 8 Wide by 12 Tall Pixels
    if (_Rotation < 0x02) {
        for (uint8_t displayRow = 0; displayRow < 12; displayRow++)	{
            if (_Rotation & 0x01) {
                _LightGrid.SetRow(displayRow, rowArray[displayRow + 12 - leftShift]);
            } else {
                _LightGrid.SetRow(11-displayRow, rowArray[displayRow + 12 - leftShift]);
            }
        }		
    }
}

// Display.cpp EOF
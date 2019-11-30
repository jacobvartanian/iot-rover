//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Planque.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Filename:	Planque.h
// Description: Utility functions to read and write data into Flash Memory.
// Author:		Danon Bradford
// Date:		2018-12-25
//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Planque.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH

#ifndef Planque_h
#define Planque_h

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <stdint.h>                 // Standard Integer Header file
#include <WString.h>                // String library for Wiring & Arduino
#define NO_GLOBAL_EEPROM 0x01u      // Only use the one instance of EEPROM here
#include <EEPROM.h>                 // EEPROM Header file

//=============================================================================
// Public Macro Definitions
//-----------------------------------------------------------------------------
#define Planque_ByteSize 512

//=============================================================================
// Class Declaration
//-----------------------------------------------------------------------------
class PlanqueClass {
    public:
    PlanqueClass() {} // Constructor
    static void Init();
    static bool AllocateVars(volatile void** variable, uint16_t const size);
    static void ReloadBufferFromFlash();
    static void WriteBufferToFlash();
    static bool NewCharArrayString(char *const addr, String const str);
    static bool NewU32(uint32_t *const addr, uint32_t const var);

    private:
    static EEPROMClass _EEPROM;
    static uint16_t _AllocationByteCount;
    static uint8_t* _BufferArray;
    
};

//=============================================================================
// Global Instance Declarations (Publicly Accessible)
//-----------------------------------------------------------------------------
extern PlanqueClass Planque;

#endif /* Planque_h */

// Planque.h EOF
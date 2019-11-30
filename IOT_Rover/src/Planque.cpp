//////////////////////////////// Planque.cpp //////////////////////////////////
// Filename:	Planque.cpp
// Description: Utility functions to read and write data into Flash Memory.
// Author:		Danon Bradford
// Date:		2018-12-25
//////////////////////////////// Planque.cpp //////////////////////////////////

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <Arduino.h>				// Arduino Header file
#include "Planque.h"				// Source Header file

//=============================================================================
// Private Macro Definitions
//-----------------------------------------------------------------------------
#define BufferIndexStart	0

//*****************************************************************************
// Publicly Accessible Global Instance Definition
//-----------------------------------------------------------------------------
PlanqueClass Planque;

//*****************************************************************************
// Externally Defined Global Instances/Variables
//-----------------------------------------------------------------------------
// none

//*****************************************************************************
// Class Member Variable Definitions (static)
//-----------------------------------------------------------------------------
EEPROMClass PlanqueClass::_EEPROM;
uint16_t PlanqueClass::_AllocationByteCount = 0x00u;
uint8_t* PlanqueClass::_BufferArray;

//=============================================================================
// Class Member Method Definitions (static)
//-----------------------------------------------------------------------------
// public:
void PlanqueClass::Init() {

    // The Arduino library called EEPROM is how we access parts of the SPI Flash
    // From a higher level, we now simply call it "Planque".
    _EEPROM.begin(Planque_ByteSize);   
    
    // Get the address of the buffer stored in RAM
    _BufferArray = _EEPROM.getDataPtr();
}

//=============================================================================
// Planque::AllocateVars
//
// Allocate space for non-volatile variable(s) of any size in the Flash memory.
// Input:
//	  variable - The address of a pointer to a variable that is to be allocated space in Flash memory.
//    size     - The byte size of the variable that will be allocated space.
// Output:
//	  bool     - true = no error.
// Conditions:
//    Planque::Init() has been called.
//-----------------------------------------------------------------------------
bool PlanqueClass::AllocateVars(volatile void** variable, uint16_t const size) {
	
    // Assume we can't allocate an address initially
  	*variable = NULL;

	// Check if there is space available
	if (size <= (Planque_ByteSize - _AllocationByteCount)) {	
		*variable = (void *)(_BufferArray + _AllocationByteCount);
		_AllocationByteCount += size;
		return true;
	}

	return false;
}

void PlanqueClass::ReloadBufferFromFlash() {
    // The EEPROM Class is quite poor... 
    // But still, it isn't worth the effor of re-writing it.
    _EEPROM.begin(Planque_ByteSize);
}

void PlanqueClass::WriteBufferToFlash() {
    // Make the dirty flag true
    (void)_EEPROM.getDataPtr();
    _EEPROM.commit();
}

bool PlanqueClass::NewCharArrayString(char *const addr, String const str) {
    if (str != String(addr)) {
        memcpy(addr, str.c_str(), str.length());
        addr[str.length()] = '\0';
        return true;
    }    
    return false;
}

bool PlanqueClass::NewU32(uint32_t *const addr, uint32_t const var) {
    if (var != *addr) {
        *addr = var;
        return true;
    }    
    return false;
}

// Planque.c EOF
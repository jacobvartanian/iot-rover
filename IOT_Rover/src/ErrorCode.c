//////////////////////////////// ErrorCode.c //////////////////////////////////
// Filename:	ErrorCode.c
// Description: Basic light weight implementation of logging errors using bits
// Author:		Danon Bradford
//////////////////////////////// ErrorCode.c //////////////////////////////////

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <stdint.h>						// Standard Integer Header file
#include "ErrorCode.h"					// Source Header file

//*****************************************************************************
// Public External Function Declarations, acting as Definitions
//-----------------------------------------------------------------------------
extern EC_Bool_t ErrorCode_12SCheck(EC_12S_t const errorCode);
extern EC_Bool_t ErrorCode_12SLog(EC_12S_t *const errorCodePtr, const uint8_t failCond);

// ErrorCode.c EOF
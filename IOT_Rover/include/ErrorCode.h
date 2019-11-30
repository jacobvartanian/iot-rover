//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH ErrorCode.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Filename:	ErrorCode.h
// Description: Basic light weight implementation of logging errors using bits
// Author:		Danon Bradford
//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH ErrorCode.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH

#ifndef DANON_ERRORCODE_H_
#define DANON_ERRORCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

//IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
//-----------------------------------------------------------------------------
#include <stdint.h>

//=============================================================================
// Public Macro Definitions
//-----------------------------------------------------------------------------
#define ErrorCode_PASS				0x00u
#define ErrorCode_FAIL				0x01u
#define ErrorCode_NONE				0x0000u	// 0b 0000 0000 0000 0000
#define ErrorCode_STAGE1			0x0001u	// 0b xxxx 0000 0000 0001
#define ErrorCode_STAGE2			0x0002u	// 0b xxxx 0000 0000 0010
#define ErrorCode_STAGE3			0x0004u	// 0b xxxx 0000 0000 0100
#define ErrorCode_STAGE4			0x0008u	// 0b xxxx 0000 0000 1000
#define ErrorCode_STAGE5			0x0010u	// 0b xxxx 0000 0001 0000
#define ErrorCode_STAGE6			0x0020u	// 0b xxxx 0000 0010 0000
#define ErrorCode_STAGE7			0x0040u	// 0b xxxx 0000 0100 0000
#define ErrorCode_STAGE8 			0x0080u	// 0b xxxx 0000 1000 0000
#define ErrorCode_STAGE9 			0x0100u	// 0b xxxx 0001 0000 0000
#define ErrorCode_STAGE10 			0x0200u	// 0b xxxx 0010 0000 0000
#define ErrorCode_STAGE11 			0x0400u	// 0b xxxx 0100 0000 0000
#define ErrorCode_STAGE12			0x0800u	// 0b xxxx 1000 0000 0000
#define ErrorCode_IncrementLOGCOUNT	0x1000u // 0b 0001 xxxx xxxx xxxx

#define ErrorCode_STAGE1PASS		(ErrorCode_IncrementLOGCOUNT + ErrorCode_NONE)
#define ErrorCode_STAGE1FAIL		(ErrorCode_IncrementLOGCOUNT + ErrorCode_STAGE1)

//=============================================================================
// Public Structure's & Type Definitions
//-----------------------------------------------------------------------------
// 12 Stage Error Code
typedef uint16_t EC_12S_t;

// Boolean Error Code
typedef uint8_t EC_Bool_t;

//=============================================================================
// Public Macro Functions
//-----------------------------------------------------------------------------

//=============================================================================
// ErrorCode_Init
//
// Initialize the ErrorCode to None/Pass.
// Input:
//	  varName - The variable name of the error code.
// Output:
//	  None.
// Conditions:
//    Can only log up to 12 errors.
//-----------------------------------------------------------------------------
#define ErrorCode_Init12StageVar(varName)	\
	varName = ErrorCode_NONE

#define ErrorCode_InitBoolVar(varName)		\
	varName = ErrorCode_PASS

#define ErrorCode_New12StageVar(varName)	\
	EC_12S_t ErrorCode_Init12StageVar(varName)

#define ErrorCode_NewBoolVar(varName)		\
	EC_Bool_t ErrorCode_InitBoolVar(varName)

//=============================================================================
// Public Inline Function Definitions, acting as symbol Declarations
//-----------------------------------------------------------------------------

//=============================================================================
// ErrorCode_12SCheck
//
// Check if the error code has logged any failed errors.
// Input:
//	  errorCode is the variable used to log errors.
// Output:
//	  ErrorCode_PASS if no logged fail errors.
//	  ErrorCode_FAIL if logged fail errors.
// Conditions:
//    Can only log up to 12 errors.
//-----------------------------------------------------------------------------
inline EC_Bool_t 
ErrorCode_12SCheck(EC_12S_t const errorCode)			
{
	if ((errorCode & 0x0FFF) == ErrorCode_NONE)
		return ErrorCode_PASS;			

	else 									
		return ErrorCode_FAIL;						
}

//============================================================================= 
// ErrorCode_12SLog 
// 
// Log that an error has occured along the stage of a function. 
// Input: 
//    errorCodePtr - Address of the variable used to log errors. 
//    failCond     - Flag to log in the error code variable. 
// Output: 
//    ErrorCode_PASS if the failCond flag was a PASS (0x00). 
//    ErrorCode_FAIL if the failCond flag was a Fail (0x01). 
// Conditions: 
//    Can only log up to 12 errors. 
//----------------------------------------------------------------------------- 
inline EC_Bool_t  
ErrorCode_12SLog(EC_12S_t *const errorCodePtr, const uint8_t failCond) 
{ 
	// Log that an error occured on the corrresponding lower 12 bits 
	if (errorCodePtr && failCond) { 
		// Acquire the log number count by shifting (*errorCodePtr) backwards by 12. 
		// This 4 bit number is the ammount to shift the error code bit 
		// into the right position (bit). 
		(*errorCodePtr) |= 0x0001 << ((*errorCodePtr) >> 12); 
	} 

	// Increment the error log number count. Note that allthough this can count up to 15, 
	// only the passFail status of the first 12 errors are recorded. 
	if (errorCodePtr && (*errorCodePtr) < 0xF000) 
		(*errorCodePtr) += ErrorCode_IncrementLOGCOUNT; 

	return (failCond & ErrorCode_FAIL); 
} 

#ifdef  __cplusplus
}
#endif

#endif /* DANON_ERRORCODE_H_ */

// ErrorCode.h EOF
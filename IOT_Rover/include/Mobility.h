// HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Mobility.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Filename:	Mobility.h
// Description: Top level module that controls moving the IDL around.
// Author:		Danon Bradford
// Date:		2019-12-07
// HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Mobility.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH

#ifndef Mobility_h
#define Mobility_h

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
// ----------------------------------------------------------------------------
#include <stdint.h>
#include "DRV8830.h"

//*****************************************************************************
// Public Macro Definitions
//-----------------------------------------------------------------------------
#define Mobility_MotorCount 2

//=============================================================================
// Public Enumerated Constants
//-----------------------------------------------------------------------------
typedef enum {
    Mobility_FrontRightMotor = 0,
    Mobility_FrontLeftMotor = 1
} Mobility_MotorIndex;

//=============================================================================
// Class Declaration
//-----------------------------------------------------------------------------
class MobilityClass {
   private:
    DRV8830Class _Motors[Mobility_MotorCount];

   public:
    MobilityClass(DRV8830_Address const addrFL, DRV8830_Address const addrFR)
        : _Motors{addrFL, addrFR} {}
    MobilityClass() : MobilityClass{DRV8830_Addr0, DRV8830_Addr1} {}
    void SetMotorAddr(Mobility_MotorIndex const index,
                      DRV8830_Address const addr) {
        _Motors[index].SetAddress(addr);
    }
    bool SetDrive(int8_t const speed, int8_t const steer);
    void PrintMotorFaults();
};

//=============================================================================
// Global Instance Declarations (Publicly Accessible)
//-----------------------------------------------------------------------------
extern MobilityClass Mobility;

#endif /* Mobility_h */

// Mobility.h EOF
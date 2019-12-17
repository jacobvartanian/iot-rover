// HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH DRV8830.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Filename:	DRV8830.h
// Description: Low-Voltage Motor Driver With I2C Interface.
// Author:		Danon Bradford
// Date:		2019-12-05
// HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH DRV8830.h HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH

#ifndef DRV8830_h
#define DRV8830_h

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// Header Files
// ----------------------------------------------------------------------------
#include <stdint.h>

//=============================================================================
// Public Enumerated Constants
//-----------------------------------------------------------------------------
typedef enum {
    DRV8830_Control = 0x00u,
    DRV8830_Fault = 0x01u
} DRV8830_Register;

typedef enum {
    DRV8830_Addr0 = 0x60u,  //  96
    DRV8830_Addr1 = 0x61u,  //  97
    DRV8830_Addr2 = 0x62u,  //  98
    DRV8830_Addr3 = 0x63u,  //  99
    DRV8830_Addr4 = 0x64u,  // 100
    DRV8830_Addr5 = 0x65u,  // 101
    DRV8830_Addr6 = 0x66u,  // 102
    DRV8830_Addr7 = 0x67u,  // 103
    DRV8830_Addr8 = 0x68u   // 104
} DRV8830_Address;

typedef enum {
    DRV8830_Coast = 0x00u,
    DRV8830_Reverse = 0x01u,
    DRV8830_Forward = 0x02u,
    DRV8830_Brake = 0x03u
} DRV8830_BridgeLogic;

//=============================================================================
// Class Declaration
//-----------------------------------------------------------------------------
class DRV8830Class {
   private:
    uint8_t _I2CAddr;
    uint8_t _ControlReg;
    bool WriteRegByte(DRV8830_Register const reg, uint8_t const byte);

   public:
    DRV8830Class(DRV8830_Address const addr) : _I2CAddr{addr} {}
    DRV8830Class() : DRV8830Class{DRV8830_Addr0} {}
    void SetAddress(DRV8830_Address const addr) { _I2CAddr = addr; }
    bool GetFaultReg(uint8_t *const dataPtr);
    bool ClearFaultReg();
    void SetBridgeControl(DRV8830_BridgeLogic const data);
    void SetVoltage(uint8_t const speed);  // 0u <= speed <= 57u
    void SetSpeedDir(int8_t const speed);  // -57 <= speed <= 57
    void SetCoast();
    bool WriteControlReg();
};

#endif /* DRV8830_h */

// DRV8830.h EOF
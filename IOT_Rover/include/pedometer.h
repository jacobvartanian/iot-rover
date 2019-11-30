/*! @file
 *
 *  @brief Routines to implement a pedometer.
 *
 *  This encapsulate the data and methods for implementing a pedometer class.
 *
 *  @author PMcL
 *  @date 2018-11-23
 */

#ifndef PEDOMETER_H
#define PEDOMETER_H

// Accelerometer library
#include "SparkFun_MMA8452Q.h"

/////////////////////////////////
// Pedometer Class Declaration //
/////////////////////////////////
class TPedometer : public MMA8452Q
{
public:
  bool StepCountHasChanged;
  bool RotationHasChanged;
  bool XYZHasChanged;
  uint32_t StepCount; //  keeps track of the number of steps
  uint8_t Rotation;

  TPedometer(); // Constructor
  uint8_t Init();
  void Update();
  uint32_t GetStepCount();
  uint8_t GetRotation();
private:
  int16_t OldX, OldY, OldZ;
  uint8_t OldRotation;
};

#endif

/*! @file
 *
 *  @brief Routines to implement a pedometer.
 *
 *  This encapsulate the data and methods for implementing a pedometer class.
 *
 *  @author PMcL
 *  @date 2018-11-23
 */

#include "pedometer.h"

const uint8_t ACCEL_ADDRESS = 0x1C;

// CONSTRUCTUR
TPedometer::TPedometer() : MMA8452Q(ACCEL_ADDRESS)
{
  OldX = 0;
  OldY = 0;
  OldZ = 0;
  OldRotation = LOCKOUT;
}

uint8_t TPedometer::Init()
{
	// Initialize our accelerometer. Set the scale to high-resolution (2g)
	// Set the output data rate to 50Hz.
  // byte status = init(SCALE_2G, ODR_50);
  // return status;
	if (!init(SCALE_2G, ODR_50))
    return false;

  // Next, we'll configure our accelerometer's tap detection interface. This is a 
  // very tweak-able function. We can configure the Threshold, time limit, and latency:

  // Pulse threshold: the threshold which is used by the system to detect
  // the start and end of a pulse.
  // Threshold can range from 1-127, with steps of 0.063g/bit.
  //byte threshold = 3; // 3 * 0.063g = 0.189g // This might work better in some cases
  const byte THRESHOLD = 2; // 2 * 0.063g = 0.063g

  // Pulse time limit: Maximum time interval that can elapse between the start of
  // the acceleration exceeding the threshold, and the end, when acceleration goes
  // below the threshold to be considered a valid pulse.
  const byte PULSE_TIME_LIMIT = 255; // 0.625 * 255 = 159ms (max)

  // Pulse latency: the time interval that starts after first pulse detection, during
  // which all other pulses are ignored. (Debounces the pulses).
  // @50Hz: Each bit adds 10ms (max: 2.56s)
  const byte PULSE_LATENCY = 64; // 1.25 * 64 = 640ms

  // Use the setupTap function to configure tap detection in our accelerometer:
  setupTap(THRESHOLD, THRESHOLD, THRESHOLD, PULSE_TIME_LIMIT, PULSE_LATENCY);    

  return true;
}

void TPedometer::Update()
{
  // Read the x, y, z values
  if (available())
  {
    OldX = x;
    OldY = y;
    OldZ = z;
    read();
    XYZHasChanged = (OldX != x) || (OldY != y) || (OldZ != z);
  }
  else
    XYZHasChanged = false;
    
	// Check the accelerometer for a tap
	if (readTap() != 0)
  {
    StepCountHasChanged = true;
		StepCount++;
  }

  // Check the accelerometer for a rotation change
  Rotation = readPL();
  RotationHasChanged = OldRotation != Rotation;
  OldRotation = Rotation;
}

uint32_t TPedometer::GetStepCount()
{
  StepCountHasChanged = false;
	return StepCount;
}

uint8_t TPedometer::GetRotation()
{
  RotationHasChanged = false;
  return Rotation;
}

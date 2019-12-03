/*! @file
 *
 *  @brief Routines to implement a distance and proximity sensor.
 *
 *  @author Jacob Vartanian
 *  @date 2019-12-03
 */

#ifndef DISTANCE_H
#define DISTANCE_H

#define DEFAULT_THRESHOLD 30
#define DEFAULT_HYSTERESIS 4

// Sensor library
#include "Adafruit_VL53L0X.h"

class Distance : private Adafruit_VL53L0X
{
public:
  Distance(); // Constructor
  uint8_t Init();
  uint16_t GetDistance();
  bool IsWithinThreshold();
  bool IsWithinRange();
private:
  void update();
  VL53L0X_RangingMeasurementData_t _measure;
  uint16_t _distance;
  uint16_t _proximityThreshold;
  uint8_t _hysteresis;
  bool _isWithinThreshold;
  bool _isWithinRange;
};

#endif // DISTANCE_H

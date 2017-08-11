/*
 * SensorConfig
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "SensorConfig.h"

// static const char *SensorDataTypeStr = { "PM", "HCHO", "CO2", "TEMP", "HUMID" };

static const char * const SensorTypeStr[] = {
  "NONE",         // 0
  "PMS5003",      // 1
  "PMS5003T",     // 2
  "PMS5003S",     // 3
  "PMS5003ST",    // 4
  "DSCO220",      //= 5,
  "MPU6050"       //= 6,
};

const char * sensorTypeStr(SensorType type)
{
  return SensorTypeStr[type];
}

uint32_t capabilityForSensorType(SensorType type)
{
  switch(type) {
    case PMS5003:
      return PM_CAPABILITY_MASK;

    case PMS5003T:
      return (PM_CAPABILITY_MASK | TEMP_HUMID_CAPABILITY_MASK);

    case PMS5003S:
      return (PM_CAPABILITY_MASK | HCHO_CAPABILITY_MASK);

    case PMS5003ST:
      return (PM_CAPABILITY_MASK | HCHO_CAPABILITY_MASK | TEMP_HUMID_CAPABILITY_MASK);

    case DSCO220:
      return CO2_CAPABILITY_MASK;

    case MPU6050:
      return ORIENTATION_CAPABILITY_MASK;

    default:
      return 0;
  }
}

uint16_t rxProtocolLengthForSensorType(SensorType type)
{
  switch(type) {
    case PMS5003:
    case PMS5003T:
    case PMS5003S:
      return 32;

    case PMS5003ST:
      return 40;

    case DSCO220:
      return 12;

    default:
      return 0;
  }
}

uint16_t tempHumidDataPosForSensorType(SensorType type)
{
  switch(type) {
    case PMS5003T:
      return 10;

    case PMS5003ST:
      return 13;

    default:
      return 0xFFFF; // invalid
  }
}
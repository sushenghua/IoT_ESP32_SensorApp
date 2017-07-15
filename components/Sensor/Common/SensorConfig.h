/*
 * SensorConfig
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SENSOR_CONFIG_H
#define _SENSOR_CONFIG_H
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////////////////
// Sensor capability mask
/////////////////////////////////////////////////////////////////////////////////////////
#define PM_CAPABILITY_MASK             0x00000001
#define HCHO_CAPABILITY_MASK           0x00000002
#define TEMP_HUMID_CAPABILITY_MASK     0x00000004
#define CO2_CAPABILITY_MASK            0x00000008
#define ORIENTATION_CAPABILITY_MASK    0x00000010


/////////////////////////////////////////////////////////////////////////////////////////
// Sensor type
/////////////////////////////////////////////////////////////////////////////////////////
enum SensorType
{
  NONE             = 0, 
  PMS5003          ,//= 1,
  PMS5003T         ,//= 2,
  PMS5003S         ,//= 3,
  PMS5003ST        ,//= 4,
  DSCO220          ,//= 5,
  MPU6050           //= 6,
};

#define PM_RX_PROTOCOL_MAX_LENGTH    40  // for PMS5003* series
#define CO2_RX_PROTOCOL_MAX_LENGTH   12  // for DS-CO2-20

const char * sensorTypeStr(SensorType type);
uint32_t capabilityForSensorType(SensorType type);
uint16_t rxProtocolLengthForSensorType(SensorType type);
uint16_t tempHumidDataPosForSensorType(SensorType type);

enum SensorDataType
{
  PM        = 0,
  HCHO      = 1,
  CO2       = 2,
  TEMP      = 3,
  HUMID     = 4
};

#endif // _SENSOR_CONFIG_H

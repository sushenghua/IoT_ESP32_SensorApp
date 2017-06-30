/*
 * SensorConfig
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SENSOR_CONFIG_H
#define _SENSOR_CONFIG_H

enum SensorDataType
{
  PM        = 0,
  HCHO      = 1,
  CO2       = 2,
  TEMP      = 3,
  HUMID     = 4
};

// const char *SensorDataTypeStr = { "PM", "HCHO", "CO2", "TEMP", "HUMID" };

#define NOT_DEFINED            -1

#ifndef PM_SENSOR_TYPE
#define PMS5003T                0
#define PMS5003                 1
#define PMS5003S                2
#define PMS5003ST               3
#define PM_SENSOR_TYPE          PMS5003S
#endif

#ifndef CO2_SENSOR_TYPE
#define DS_CO2_20               0
#define CO2_SENSOR_TYPE         NOT_DEFINED
#endif

#endif // _SENSOR_CONFIG_H

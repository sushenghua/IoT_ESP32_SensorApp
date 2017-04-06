/*
 * OrientationSensor: communicate with MPU6050 sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _ORIENTATION_SENSOR_H
#define _ORIENTATION_SENSOR_H

#include "../../ExtInterrupt/ExtInterrupt.h"
#include "../../DisplayController/SensorDisplayController.h"
#include "MPU6050/MPU6050.h"

#define CO2_RX_PROTOCOL_LENGTH  12
#define CO2_RX_BUF_CAPACITY     CO2_RX_PROTOCOL_LENGTH

class OrientationSensor : public ExtInterrupt
{
public:
	// constructor
	OrientationSensor(uint16_t pin)
	: ExtInterrupt(pin)
	{ registerCallback(this); }

	void init();

	void setDisplayDelegate(SensorDisplayController *dc) { _dc = dc; }

	// called on every interruption
	virtual void tick() {
			if (_dc) {
					_updateOrientation();
			}
	}
protected:
	// helper
	void _updateOrientation();

protected:
	uint16_t                  _pin;
	SensorDisplayController  *_dc;
	MPU6050Data               _mpuData;
};

#endif // _ORIENTATION_SENSOR_H

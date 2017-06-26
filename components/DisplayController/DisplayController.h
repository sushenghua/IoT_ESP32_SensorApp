/*
 * DisplayController: manage content display on device
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _DISPLAY_CONTROLLER_H
#define _DISPLAY_CONTROLLER_H

#include "DisplayGFX.h"

class DisplayController
{
public:
    static DisplayController * activeInstance();

    DisplayController(DisplayGFX *dev);

public:
    virtual void init() {};
    virtual void tick() {};
    virtual void update() = 0;

    void reset() { _dev->reset(); }
    void turnOn(bool on) { _dev->turnOn(on); }

protected:
	void displayStatusBar();

protected:
	bool        _showWifi;
	bool        _showTime;
	bool        _showBattery;
	uint16_t    _contentOffsetY;
    DisplayGFX *_dev;
};

#endif // _DISPLAY_CONTROLLER_H

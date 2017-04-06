/*
 * DisplayController: manage content display on oled device
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _DISPLAY_CONTROLLER_H
#define _DISPLAY_CONTROLLER_H

#include "DisplayGFX.h"

class DisplayController
{
public:
    DisplayController(DisplayGFX *dev): _dev(dev) {}

public:
    virtual void init() {};
    virtual void tick() {};
    virtual void update() = 0;

protected:
    DisplayGFX *_dev;
};

#endif // _DISPLAY_CONTROLLER_H

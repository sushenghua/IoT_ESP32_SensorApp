/*
 * Semaphore: predefined static semaphore
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SEMAPHORE_CLASS_H
#define _SEMAPHORE_CLASS_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class Semaphore
{
public:
    static void init();
    static void deinit();

public:
    static xSemaphoreHandle i2c;
};

#endif // _SEMAPHORE_CLASS_H

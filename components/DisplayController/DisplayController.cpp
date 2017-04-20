/*
 * DisplayController: manage content display on device
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "DisplayController.h"

static DisplayController *_activeDisplayController = NULL;

DisplayController * DisplayController::activeInstance()
{
	return _activeDisplayController;
}

DisplayController::DisplayController(DisplayGFX *dev)
: _dev(dev)
{
	_activeDisplayController = this;
}

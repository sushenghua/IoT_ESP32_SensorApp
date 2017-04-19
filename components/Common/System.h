/*
 * System.h system function
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

class System
{
public:
	static const char* macAddress();
	static const char* idfVersion();
	static const char* firmwareVersion();

	static void restart();

	static bool restartInProgress();
};

#endif // _SYSTEM_H_

#pragma once

#include "IOCPModule.h"

class Core
{
public:
	static Core* getInstance();
	Core();
	~Core();
	void start();
private:
	static Core* instance;
	static void threadproc();

	static IOCPModule* iocpModule;
	int stop = 0;
	int processorsNum = 0;
};


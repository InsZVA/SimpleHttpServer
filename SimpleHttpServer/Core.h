#pragma once
#include <thread>
#include "IOCPModule.h"

#ifdef DEBUG
#include <iostream>
#endif
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

	int stop = 0;
	int processorsNum = 0;
};


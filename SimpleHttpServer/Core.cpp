#include "stdafx.h"
#include "Core.h"
#include <thread>
#include <Windows.h>

//#define DEBUG

#ifdef DEBUG
#include <iostream>
#endif

Core* Core::instance = nullptr;
IOCPModule* Core::iocpModule = nullptr;

Core::Core()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	processorsNum = si.dwNumberOfProcessors;

	iocpModule = IOCPModule::getInstance();
	if (!iocpModule->initialize())
	{
		iocpModule->~IOCPModule();
		delete iocpModule;
#ifdef DEBUG
		system("pause");
#endif
		exit(-2);
	}
}


Core::~Core()
{
}

Core* Core::getInstance() {
	if (instance) return instance;
	return instance = new Core();
}

void Core::start() {
	std::thread** t = new std::thread*[processorsNum];
	for (auto i = 0; i < processorsNum; i++) {
		t[i] = new std::thread(Core::threadproc);
	}
	for (auto i = 0; i < processorsNum; i++) {
		t[i]->join();
	}
}

void Core::threadproc() {
#ifdef DEBUG
	std::cout << "Worker thread start..." << std::endl;
#endif
	for (; iocpModule->eventLoop();) {
	}
#ifdef DEBUG
	std::cout << "I'm exploding..." << std::endl;
#endif
}
#include "stdafx.h"
#include "Core.h"

//#define DEBUG


Core* Core::instance = nullptr;

Core::Core()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	processorsNum = si.dwNumberOfProcessors;

	if (!IOCPModule::initialize()) {
#ifdef DEBUG
		std::cout << "IOCP Module initialize failed!" << std::endl;
#endif
		exit(-1);
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
	//processorsNum = 1; //for debug

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
	IOCPModule *iocpModule = new IOCPModule(new Mempool());
	for (; ;) {
		if (!iocpModule->eventLoop()) {
			std::cout << "debug" << std::endl;
		}
	}
#ifdef DEBUG
	std::cout << "I'm exploding..." << std::endl;
#endif
}
// SimpleHttpServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define TEST
#include "Mempool.h"
#include "AnalyseUtil.h"
#include "Core.h"
#ifndef TEST
#include <iostream>
#endif


#define ASSERT(b, s) if (!(b)) { \
	std::cout << "ASSERT:" << (s) << std::endl; \
	return -1; \
}

void printMem(void* start, int len) {
	for (unsigned int* i = (unsigned int*)start; i <
		(unsigned int*)((char*)start + len); i++)
	{
		printf("%X ", *i);
	}
	printf("\n");
}

int test() {
	Mempool* mempool = new Mempool();
	auto analyse = AnalyseUtil::getInstance();
#define PARALLEL 4096
#define LOOP_NUM (64*1024/PARALLEL)
	void* p[4096];
	auto h = analyse->suspend();
	for (auto i = 0; i < LOOP_NUM; i++) {
		for (auto j = 0; j < PARALLEL; j++)
			p[j] = malloc(sizeof(IOCPModule::PER_SOCKET_CONTEXT));
		for (auto j = 0; j < PARALLEL; j++)
			free(p[j]);
	}
	analyse->recover(h, "malloc");
	h = analyse->suspend();
	for (auto i = 0; i < LOOP_NUM; i++) {
		for (auto j = 0; j < PARALLEL; j++)
			p[j] = mempool->alloc(sizeof(IOCPModule::PER_SOCKET_CONTEXT));
		for (auto j = 0; j < PARALLEL; j++)
			mempool->free(p[j]);
	}
	analyse->recover(h, "pool");
	analyse->printTable(stdout);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef TEST
	return test();
#else
	Core* core = Core::getInstance();
	core->start();
	return 0;
#endif
}
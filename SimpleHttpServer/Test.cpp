#include "stdafx.h"
#include "Test.h"


#define ASSERT(b, s) if (!(b)) { \
	std::cout << "ASSERT:" << (s) << std::endl; \
	return -1; \
}

void Test::printMem(const void* start, int len) {
	for (unsigned int* i = (unsigned int*)start; i <
		(unsigned int*)((char*)start + len); i++)
	{
		printf("%X ", *i);
	}
	printf("\n");
}

int Test::TestAll() {
	auto ret = TestMempool();
	if (!ret) {
		std::cout << "All tests PASS!" << std::endl;
	}
	system("pause");
	return ret;
}

int Test::TestMempool() {
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
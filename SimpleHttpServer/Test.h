#pragma once
#include <iostream>
#include "Mempool.h"
#include "AnalyseUtil.h"
#include "IOCPModule.h"
class Test
{
public:
	Test() = delete;
	~Test() = delete;
	static int TestAll();
	static int TestMempool();
	static void printMem(const void* start, int len);
};


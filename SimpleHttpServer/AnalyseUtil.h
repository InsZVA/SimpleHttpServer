#pragma once

#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <fstream>
#include <sstream>

#define MAX_HANDLER 256

class AnalyseUtil
{
public:
	typedef unsigned char HANDLE;

	// TIME struct include int and double
	// to avoid double accuracy loss
	typedef struct _TIME {
		int nSeconds;
		double tail;
	} TIME;

	static AnalyseUtil* getInstance();
	HANDLE suspend();
	void recover(HANDLE h, std::string);

	// Print analyse table to a file
	void printTable(std::string path);
	void printTable(FILE*);
private:
	std::atomic<HANDLE> handle = 0;
	long long startTime[MAX_HANDLER];
	std::mutex suspendTimeTableMutex;
	std::map<std::string, TIME> suspendTimeTable;

	static AnalyseUtil* instance;
};


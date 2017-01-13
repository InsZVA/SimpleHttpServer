#pragma once

#include <string>
#include <map>
#include <atomic>
#include <mutex>

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

	AnalyseUtil() = delete;
	~AnalyseUtil();
	static HANDLE suspend();
	static void recover(HANDLE h, std::string);

	// Print analyse table to a file
	static void printTable();
private:
	static std::atomic<HANDLE> handle;
	static long long startTime[MAX_HANDLER];
	static std::mutex suspendTimeTableMutex;
	static std::map<std::string, TIME> suspendTimeTable;
};


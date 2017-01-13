#include "stdafx.h"
#include "AnalyseUtil.h"
#include <WinBase.h>
#include <fstream>
#include <sstream>

AnalyseUtil::~AnalyseUtil()
{
}

inline AnalyseUtil::HANDLE AnalyseUtil::suspend() {
	auto h = handle++;
	LARGE_INTEGER m_liPerfStart = { 0 };
	QueryPerformanceCounter(&m_liPerfStart);
	startTime[h] = m_liPerfStart.QuadPart;
}

inline void AnalyseUtil::recover(HANDLE h, std::string name) {
	LARGE_INTEGER m_liPerfFreq = { 0 };
	QueryPerformanceFrequency(&m_liPerfFreq);
	LARGE_INTEGER m_liPerfEnd = { 0 };
	QueryPerformanceCounter(&m_liPerfEnd);

	std::lock_guard<std::mutex> lock(suspendTimeTableMutex);
	suspendTimeTable[name].tail = suspendTimeTable[name].tail + 
		(double)(m_liPerfEnd.QuadPart - startTime[h]) / (double)m_liPerfFreq.QuadPart;
	if (suspendTimeTable[name].tail > 1) {
		suspendTimeTable[name].tail--;
		suspendTimeTable[name].nSeconds++;
	}
}

void AnalyseUtil::printTable() {
	std::ostringstream oss;
	std::ofstream of("./analyse.txt");
	// TODO: MapFile and write
	suspendTimeTableMutex.lock();
	for (auto& m : suspendTimeTable) {
		oss << m.first << "\t\t\t" << m.second.nSeconds << "s+" << m.second.tail << std::endl;
	}
	suspendTimeTableMutex.unlock();
	of << oss.str();
}
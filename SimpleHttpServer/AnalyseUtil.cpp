#include "stdafx.h"
#include "AnalyseUtil.h"


AnalyseUtil* AnalyseUtil::instance = nullptr;

AnalyseUtil* AnalyseUtil::getInstance() {
	if (!instance) instance = new AnalyseUtil();
	return instance;
}

AnalyseUtil::HANDLE AnalyseUtil::suspend() {
	auto h = handle++;
	LARGE_INTEGER m_liPerfStart = { 0 };
	QueryPerformanceCounter(&m_liPerfStart);
	startTime[h] = m_liPerfStart.QuadPart;
	return h;
}

void AnalyseUtil::recover(HANDLE h, std::string name) {
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

void AnalyseUtil::printTable(std::string path = "./analyse.txt") {
	std::ostringstream oss;
	std::ofstream of(path);
	// TODO: MapFile and write
	suspendTimeTableMutex.lock();
	for (auto& m : suspendTimeTable) {
		oss << m.first << "\t\t\t" << m.second.nSeconds << "s+" << m.second.tail << std::endl;
	}
	suspendTimeTableMutex.unlock();
	of << oss.str();
}

void AnalyseUtil::printTable(FILE* f) {
	std::ostringstream oss;
	std::ofstream of(f);
	// TODO: MapFile and write
	suspendTimeTableMutex.lock();
	for (auto& m : suspendTimeTable) {
		oss << m.first << "\t\t\t" << m.second.nSeconds << "s+" << m.second.tail << std::endl;
	}
	suspendTimeTableMutex.unlock();
	of << oss.str();
}
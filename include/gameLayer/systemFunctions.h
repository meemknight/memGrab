#pragma once
#include <string>
#include <vector>
#include "program.h"
#include "errorLogging.h"
#include <sstream>

struct ProcessWindow
{
	std::string windowName;
	std::string processName;
	PID pid = 0;
};

PID findPidByName(const char* name);
bool isProcessAlive(PROCESS process);

std::vector<std::pair<std::string, PID>> getAllProcesses();
std::vector<ProcessWindow> getAllWindows();

std::string printAllProcesses(PID& pid);
std::string printAllWindows(PID& pid);

//returns 0 on fail
PROCESS openProcessFromPid(PID pid);
void closeProcess(PROCESS process);

std::string getLastErrorString();



//memore manipulation////////////////////////////////////////////////////////////////////////////////////////////////////////
void writeMemory(PROCESS process, void *ptr, void* data, size_t size, ErrorLog &errorLog);
bool readMemory(PROCESS process, void *start, size_t size, void *buff);

enum
{
	memQueryFlags_ = 0,
	memQueryFlags_None = 0,
	memQueryFlags_Read		= 0b0001,
	memQueryFlags_Write		= 0b0010,
	memQueryFlags_Execute	= 0b0100,


};

#if defined WIN32 || defined _WIN32 || defined __WIN32__ || defined __NT__

struct OppenedQuery
{
	PROCESS queriedProcess = 0;
	char *baseQueriedPtr = (char *)0x0;
	bool oppened() { return queriedProcess != 0; }
};

#endif

#if defined __linux__

struct OppenedQuery
{
	std::stringstream mapData;
	bool oppened() { return mapData.rdbuf()->in_avail() != 0; }
};

#endif


OppenedQuery initVirtualQuery(PROCESS process);
bool getNextQuery(OppenedQuery &query, void* &low, void* &hi, int& flags);

std::vector<void*> findBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen);

void refindBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen, std::vector<void*>& found);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
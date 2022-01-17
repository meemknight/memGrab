#pragma once
#include <string>
#include <vector>
#include "program.h"
#include "errorLogging.h"

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
void writeMemory(PROCESS process, void *ptr, void* data, size_t size, ErrorLog &errorLog);
bool readMemory(PROCESS process, void *start, size_t size, void *buff);

bool initVirtualQuery(PROCESS process);
bool getNextQuery(void **low, void **hi);

std::vector<void*> findBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen);



void refindBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen, std::vector<void*>& found);

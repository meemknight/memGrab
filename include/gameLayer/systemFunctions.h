#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "program.h"
#include "errorLogging.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	using PID = DWORD;
	using PROCESS = HANDLE;
#elif definded(__linux__)

#endif

PID findPidByName(const char* name);
bool isProcessAlive(PROCESS process);
std::string printAllProcesses(PID& pid);
std::string printAllWindows(PID& pid);
std::string getLastErrorString();
void writeMemory(PROCESS process, void *ptr, void* data, size_t size, ErrorLog &errorLog);
std::vector<void*> findBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen);
void refindBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen, std::vector<void*>& found);

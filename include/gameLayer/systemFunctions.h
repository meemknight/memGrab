#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "program.h"
#include "errorLogging.h"

DWORD findPidByName(const char* name);
bool isProcessAlive(HANDLE process);
std::string printAllProcesses(DWORD& pid);
std::string printAllWindows(DWORD& pid);
std::string getLastErrorString();
void writeMemory(HANDLE process, void *ptr, void* data, size_t size, ErrorLog &errorLog);
std::vector<void*> findBytePatternInProcessMemory(HANDLE process, void* pattern, size_t patternLen);
void refindBytePatternInProcessMemory(HANDLE process, void* pattern, size_t patternLen, std::vector<void*>& found);

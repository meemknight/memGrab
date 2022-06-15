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




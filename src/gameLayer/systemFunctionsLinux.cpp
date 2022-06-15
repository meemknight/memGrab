#if defined __linux__
#include "systemFunctions.h"
#include "imgui.h"
#include <vector>
#include <algorithm>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <filesystem>

//PID is just a number
//PROCESS is a handle in windows
//todo figure out what should it be in linux? mabe also a pid


//returns 0 on failure
//probably remove
PID findPidByName(const char* name)
{

	return 0;
}

//returns 0 if not alive or not existing
bool isProcessAlive(PROCESS process)
{
	struct stat sts = {};

	// creating a string and pass it as a parameter for the stat function
	std::string passingString = "/proc/";

	// add the process parameter to the string
	passingString += std::to_string(process);

	// https://stackoverflow.com/questions/9152979/check-if-process-exists-given-its-pid
	if(stat(passingString.c_str(), &sts) == -1 && errno == ENOENT)
	{
		// this means that the process doesn't exist
		return 0;
	}

	return 1;
	
}

bool isNumber(const std::string& str)
{
	for (char const &c : str)
	{
		if (std::isdigit(c) == 0) return false;
	}
	return true;
}

//gets a list of all the processes, you should ignore the system process(the one with pid 0)
std::vector<std::pair<std::string, PID>> getAllProcesses()
{
	std::vector<std::pair<std::string, PID>> returnVector;
	for(const auto &i: std::filesystem::directory_iterator("/proc"))
	{
		std::pair<std::string, PID> entry;
		entry.first = i.path().filename();

		if (isNumber(entry.first))
		{
			entry.second = std::atoi(entry.first.c_str());

			auto path = "/proc/" + entry.first + "/comm";

			std::ifstream file(path);
			std::string name;
			file >> name;
			file.close();
			entry.first = name;

			returnVector.push_back(std::move(entry));
		}
	}

	return returnVector;
}

//gets a list of all windows, you should ignore the system process(the one with pid 0)
std::vector<ProcessWindow> getAllWindows()
{
	return {};
}

//gets last error as a string
std::string getLastErrorString()
{
	return strerror(errno);
}


//returns 0 on fail
//on linux will probably just return the pid or sthing
PROCESS openProcessFromPid(PID pid)
{
	return pid;
}

void closeProcess(PROCESS process)
{
	//do nothing
}

#endif
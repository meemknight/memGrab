#if defined __linux__
#include "systemFunctions.h"
#include "imgui.h"
#include <vector>
#include <algorithm>


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
	return 0;
}

//gets a list of all the processes, you should ignore the system process(the one with pid 0)
std::vector<std::pair<std::string, PID>> getAllProcesses()
{
	return {};
}

//gets a list of all windows, you should ignore the system process(the one with pid 0)
std::vector<ProcessWindow> getAllWindows()
{
	return {};
}

//gets last error as a string
std::string getLastErrorString()
{
	return "";
}


//writes memory in the process' space
void writeMemory(PROCESS process, void* ptr, void* data, size_t size, ErrorLog& errorLog)
{

	//if (!writeSucceeded) //exaple setting error
	//{
	//	errorLog.setError(getLastErrorString().c_str());
	//}
}


//scans the process for the byte patern
std::vector<void*> findBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen)
{

	return{};
}

//returns 0 on fail
//on linux will probably just return the pid or sthing
PROCESS openProcessFromPid(PID pid)
{
	return pid; //?
}

void closeProcess(PROCESS process)
{
	//do nothing

}

#endif
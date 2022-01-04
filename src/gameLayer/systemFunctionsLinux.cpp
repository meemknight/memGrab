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
#include <sstream>

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
	return strerror(errno);
}


//writes memory in the process' space
void writeMemory(PROCESS process, void* ptr, void* data, size_t size, ErrorLog& errorLog)
{

	//https://nullprogram.com/blog/2016/09/03/

	errorLog.clearError();

	char file[256]={};
	sprintf(file, "/proc/%ld/mem", (long)process);
	int fd = open(file, O_RDWR);


	if(fd == -1)
	{
		errorLog.setError(getLastErrorString().c_str());
		return;
	}

	if(ptrace(PTRACE_ATTACH, process, 0, 0) == -1)
	{
		errorLog.setError(getLastErrorString().c_str());
		close(fd);
		return;
	}

	//wait for process to change state
	if(waitpid(process, NULL, 0) == -1)
	{
		errorLog.setError(getLastErrorString().c_str());
		ptrace(PTRACE_DETACH, process, 0, 0);
		close(fd);
		return;
	}

	off_t addr = (off_t)ptr; // target process address

	if(pwrite(fd, data, size, addr) == -1)
	{
		errorLog.setError(getLastErrorString().c_str());
		ptrace(PTRACE_DETACH, process, 0, 0);
		close(fd);
		return;
	}

	ptrace(PTRACE_DETACH, process, 0, 0);
	close(fd);

	//if (!writeSucceeded) //exaple setting error
	//{
	//	errorLog.setError(getLastErrorString().c_str());
	//}
}

std::stringstream mapData;

int mapsInit(pid_t pid)
{
	mapData.clear();

	char fileName[256]={};
	sprintf(fileName, "/proc/%ld/maps", (long)pid);
	
	std::ifstream file(fileName);
	if(!file.is_open())	{return -1;}

	std::vector<char> data{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
	mapData = std::stringstream(std::string(data.begin(), data.end()));

	file.close();

	return 1;
}


void mapClose(int* fd)
{
	close(*fd);
}

bool mapsNext(void** low, void** hi)
{
	if(mapData.eof()){return false;}

	std::string adress;
	std::string permisions;
	size_t offset;
	std::string device;
	long inode;
	std::string pathName;

	mapData >> adress >> permisions >> offset >> device >> inode >> pathName;

	auto pos = adress.find('-');
	std::string beg(adress.begin(), adress.begin() + pos);
	std::string end(adress.begin() + pos + 1, adress.end());

	*low = (void*)atoll(beg.c_str()); 
	*hi = (void*)atoll(end.c_str()); 

	return true;
}

//scans the process for the byte patern
std::vector<void*> findBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen)
{
	if (patternLen == 0) { return {}; }

	std::vector<void*> returnVec;
	returnVec.reserve(1000);

	char* basePtr = (char*)0x0;

	if(mapsInit(process) < 0)
    	return {};

	void* low;
	void* hi;

	while (mapsNext(&low, &hi))
	{
		
		//search for our patern
		char* remoteMemRegionPtr = (char*)low;
		char* localCopyContents = new char[(char*)hi-(char*)low];
		size_t bytesRead = 0;
		if (ReadProcessMemory(process, memInfo.BaseAddress, localCopyContents, memInfo.RegionSize, &bytesRead))
		{
			char* cur = localCopyContents;
			size_t curPos = 0;
			while (curPos < memInfo.RegionSize - patternLen + 1)
			{
				if (memcmp(cur, pattern, patternLen) == 0)
				{
					returnVec.push_back((char*)memInfo.BaseAddress + curPos);
				}
				curPos++;
				cur++;
			}
		}
		delete[] localCopyContents;

		basePtr = (char*)memInfo.BaseAddress + memInfo.RegionSize;
	}

	return returnVec;
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
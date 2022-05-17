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


bool readMemory(PROCESS process, void* start, size_t size, void *buff)
{
	char file[256]={};
	sprintf(file, "/proc/%ld/mem", (long)process);
	int fd = open(file, O_RDWR);

	if(fd == -1)
	{
		return 0;
	}

	if(ptrace(PTRACE_ATTACH, process, 0, 0) == -1)
	{
		close(fd);
		return 0;
	}

	if(waitpid(process, NULL, 0) == -1)
	{
		ptrace(PTRACE_DETACH, process, 0, 0);
		close(fd);
		return 0;
	}

	off_t addr = (off_t)start; // target process address

	if(pread(fd, buff, size, addr) == -1)
	{
		ptrace(PTRACE_DETACH, process, 0, 0);
		close(fd);
		return 0;
	}

	ptrace(PTRACE_DETACH, process, 0, 0);
	close(fd);

	return 1;
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


OppenedQuery initVirtualQuery(PROCESS process)
{
	OppenedQuery query{};

	char fileName[256]={};
	sprintf(fileName, "/proc/%ld/maps", (long)process);
	
	std::ifstream file(fileName);
	if(!file.is_open())	{return query; /*fail*/}

	std::vector<char> data{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
	query.mapData = std::stringstream(std::string(data.begin(), data.end()));

	file.close();

	return query;
}

bool getNextQuery(OppenedQuery &query, void *&low, void *&hi, int &flags)
{
	flags = memQueryFlags_Read | memQueryFlags_Write | memQueryFlags_Comitted;
	
	if(query.mapData.eof()){query = OppenedQuery(); return false;}

	std::string line;
	std::getline(query.mapData, line);

	std::stringstream lineStream(line);

	std::string adress;
	std::string permisions;
	std::string offset;
	std::string device;
	std::string inode;
	std::string pathName;

	lineStream >> adress >> permisions >> offset >> device >> inode >> pathName;

	auto pos = adress.find('-');

	if(pos == adress.npos)
	{
		return false;
	}

	std::string beg(adress.begin(), adress.begin() + pos);
	std::string end(adress.begin() + pos + 1, adress.end());

	size_t lowVal = std::stoull(beg, 0, 16);
	size_t highVal = std::stoull(end, 0, 16);

	low = (void*)lowVal; 
	hi = (void*)highVal;

	return true;
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
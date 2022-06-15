#include "virtualQueryApi.h"
#include <algorithm>

void refindBytePatternInProcessMemory(PROCESS process, void *pattern, size_t patternLen, std::vector<void *> &found)
{
	if (patternLen == 0) { return; }

	auto newFound = findBytePatternInProcessMemory(process, pattern, patternLen);

	std::vector<void *> intersect;
	intersect.resize(std::min(found.size(), newFound.size()));

	std::set_intersection(found.begin(), found.end(),
		newFound.begin(), newFound.end(),
		intersect.begin());

	intersect.erase(std::remove(intersect.begin(), intersect.end(), nullptr), intersect.end());

	found = std::move(intersect);
}

//http://kylehalladay.com/blog/2020/05/20/Rendering-With-Notepad.html
std::vector<void *> findBytePatternInProcessMemory(PROCESS process, void *pattern, size_t patternLen)
{
	if (patternLen == 0) { return {}; }

	std::vector<void *> returnVec;
	returnVec.reserve(1000);

	auto query = initVirtualQuery(process);

	if (!query.oppened());
	return {};

	void *low = nullptr;
	void *hi = nullptr;
	int flags = memQueryFlags_None;


	while (getNextQuery(query, low, hi, flags))
	{
		if ((flags & memQueryFlags_Comitted) && (flags & memQueryFlags_Read) && (flags & memQueryFlags_Write))
		{
			//search for our byte patern
			size_t size = (char *)hi - (char *)low + 1;
			char *localCopyContents = new char[size];
			if (
				readMemory(process, low, size, localCopyContents)
				)
			{
				char *cur = localCopyContents;
				size_t curPos = 0;
				while (curPos < size - patternLen + 1)
				{
					if (memcmp(cur, pattern, patternLen) == 0)
					{
						returnVec.push_back((char *)low + curPos);
					}
					curPos++;
					cur++;
				}
			}
			delete[] localCopyContents;
		}
	}

	return returnVec;
}

#if defined WIN32 || defined _WIN32 || defined __WIN32__ || defined __NT__

#include <Windows.h>
#include <TlHelp32.h>
#include "systemFunctions.h"
#include "imgui.h"
#include <vector>
#include <algorithm>

#undef min
#undef max





OppenedQuery initVirtualQuery(PROCESS process)
{
	OppenedQuery q = {};

	q.queriedProcess = process;
	q.baseQueriedPtr = 0;
	return q;
}

bool getNextQuery(OppenedQuery &query, void *&low, void *&hi, int &flags)
{

	if (query.queriedProcess == 0) { return false; }

	flags = memQueryFlags_None;
	low = nullptr;
	hi = nullptr;

	MEMORY_BASIC_INFORMATION memInfo;

	bool rez = 0;
	while (true)
	{
		rez = VirtualQueryEx(query.queriedProcess, (void *)query.baseQueriedPtr, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));

		if (!rez)
		{
			query = {};
			return false;
		}

		query.baseQueriedPtr = (char *)memInfo.BaseAddress + memInfo.RegionSize;

		if (memInfo.State == MEM_COMMIT)
		{
			flags = memQueryFlags_Comitted;

			//good page
			if (memInfo.Protect == PAGE_READONLY)
			{
				flags |= memQueryFlags_Read;
			}
			else if (memInfo.Protect == PAGE_READWRITE)
			{
				flags |= (memQueryFlags_Read | memQueryFlags_Write);
			}
			else if (memInfo.Protect == PAGE_EXECUTE)
			{
				flags |= memQueryFlags_Execute;
			}
			else if (memInfo.Protect == PAGE_EXECUTE_READ)
			{
				flags |= (memQueryFlags_Execute | memQueryFlags_Read);
			}
			else if (memInfo.Protect == PAGE_EXECUTE_READWRITE)
			{
				flags |= (memQueryFlags_Execute | memQueryFlags_Read | memQueryFlags_Write);
			}
		}

		low = memInfo.BaseAddress;
		hi = (char *)memInfo.BaseAddress + memInfo.RegionSize;
		return true;
	}
}




#endif


#if defined __linux__

bool readMemory(PROCESS process, void *start, size_t size, void *buff)
{
	char file[256] = {};
	sprintf(file, "/proc/%ld/mem", (long)process);
	int fd = open(file, O_RDWR);

	if (fd == -1)
	{
		return 0;
	}

	if (ptrace(PTRACE_ATTACH, process, 0, 0) == -1)
	{
		close(fd);
		return 0;
	}

	if (waitpid(process, NULL, 0) == -1)
	{
		ptrace(PTRACE_DETACH, process, 0, 0);
		close(fd);
		return 0;
	}

	off_t addr = (off_t)start; // target process address

	if (pread(fd, buff, size, addr) == -1)
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
void writeMemory(PROCESS process, void *ptr, void *data, size_t size, ErrorLog &errorLog)
{

	//https://nullprogram.com/blog/2016/09/03/

	errorLog.clearError();

	char file[256] = {};
	sprintf(file, "/proc/%ld/mem", (long)process);
	int fd = open(file, O_RDWR);


	if (fd == -1)
	{
		errorLog.setError(getLastErrorString().c_str());
		return;
	}

	if (ptrace(PTRACE_ATTACH, process, 0, 0) == -1)
	{
		errorLog.setError(getLastErrorString().c_str());
		close(fd);
		return;
	}

	//wait for process to change state
	if (waitpid(process, NULL, 0) == -1)
	{
		errorLog.setError(getLastErrorString().c_str());
		ptrace(PTRACE_DETACH, process, 0, 0);
		close(fd);
		return;
	}

	off_t addr = (off_t)ptr; // target process address

	if (pwrite(fd, data, size, addr) == -1)
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

	char fileName[256] = {};
	sprintf(fileName, "/proc/%ld/maps", (long)process);

	std::ifstream file(fileName);
	if (!file.is_open()) { return query; /*fail*/ }

	std::vector<char> data{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
	query.mapData = std::stringstream(std::string(data.begin(), data.end()));

	file.close();

	return query;
}

bool getNextQuery(OppenedQuery &query, void *&low, void *&hi, int &flags)
{
	flags = memQueryFlags_Read | memQueryFlags_Write | memQueryFlags_Comitted;

	if (query.mapData.eof()) { query = OppenedQuery(); return false; }

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

	if (pos == adress.npos)
	{
		return false;
	}

	std::string beg(adress.begin(), adress.begin() + pos);
	std::string end(adress.begin() + pos + 1, adress.end());

	size_t lowVal = std::stoull(beg, 0, 16);
	size_t highVal = std::stoull(end, 0, 16);

	low = (void *)lowVal;
	hi = (void *)highVal;

	return true;
}



#endif
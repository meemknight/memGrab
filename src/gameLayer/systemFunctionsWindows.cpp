//http://kylehalladay.com/blog/2020/05/20/Hooking-Input-Snake-In-Notepad.html
//https://github.com/milkdevil/injectAllTheThings

#if defined WIN32 || defined _WIN32 || defined __WIN32__ || defined __NT__

#include <Windows.h>
#include <TlHelp32.h>
#include "systemFunctions.h"
#include "imgui.h"
#include <vector>
#include <algorithm>

#undef min
#undef max

PID findPidByName(const char* name)
{
	HANDLE h;
	PROCESSENTRY32 singleProcess;
	h = CreateToolhelp32Snapshot( //takes a snapshot of specified processes
		TH32CS_SNAPPROCESS, //get all processes
		0); //ignored for SNAPPROCESS

	singleProcess.dwSize = sizeof(PROCESSENTRY32);

	do
	{
		if (strcmp(singleProcess.szExeFile, name) == 0)
		{
			DWORD pid = singleProcess.th32ProcessID;
			CloseHandle(h);
			return pid;
		}

	} while (Process32Next(h, &singleProcess));

	CloseHandle(h);

	return 0;
}



//https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror
std::string getLastErrorString()
{
	//Get the error message ID, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
	{
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}

void writeMemory(PROCESS process, void* ptr, void* data, size_t size, ErrorLog& errorLog)
{

	errorLog.clearError();

	BOOL writeSucceeded = WriteProcessMemory(
		process,
		ptr,
		data,
		size,
		NULL);

	if (!writeSucceeded)
	{
		errorLog.setError(getLastErrorString().c_str());
	}

}


bool isProcessAlive(PROCESS process)
{
	DWORD exitCode = 0;

	BOOL code = GetExitCodeProcess(process, &exitCode);

	return code && (exitCode == STILL_ACTIVE);
}

std::vector<std::pair<std::string, PID>> getAllProcesses()
{
	HANDLE h;
	PROCESSENTRY32 singleProcess = {sizeof(PROCESSENTRY32)};
	h = CreateToolhelp32Snapshot( //takes a snapshot of specified processes
		TH32CS_SNAPPROCESS, //get all processes
		0); //ignored for SNAPPROCESS

	std::vector<std::pair<std::string, PID>> returnVector;
	returnVector.reserve(500);

	while (Process32Next(h, &singleProcess))
	{
		if (singleProcess.th32ProcessID == 0) { continue; } //ignore system process

		std::pair<std::string, PID> process;

		process.first = singleProcess.szExeFile;
		process.second = singleProcess.th32ProcessID;

		returnVector.push_back(std::move(process));
	}

	CloseHandle(h);

	return returnVector;
}

std::vector<ProcessWindow> allWindows;

void setNameOfProcesses()
{
	HANDLE h;
	PROCESSENTRY32 singleProcess = {sizeof(PROCESSENTRY32)};
	h = CreateToolhelp32Snapshot( //takes a snapshot of specified processes
		TH32CS_SNAPPROCESS, //get all processes
		0); //ignored for SNAPPROCESS

	while (Process32Next(h, &singleProcess))
	{
		auto name = singleProcess.szExeFile;
		auto pid = singleProcess.th32ProcessID;

		auto f = std::find_if(allWindows.begin(), allWindows.end(), [pid](ProcessWindow& i) { return i.pid == pid; });

		if (f != allWindows.end())
		{
			int index = f - allWindows.begin();

			allWindows[index].processName = name;

		}

	}

	for (int i = 0; i < allWindows.size(); i++)
	{
		if (allWindows[i].processName == "---notFound?---")
		{
			allWindows.erase(allWindows.begin() + i);
			i--;
		}
	}


	CloseHandle(h);
}

BOOL CALLBACK EnumWindowsProc(
	HWND   hwnd,
	LPARAM lParam
)
{
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);


	if (pid && IsWindowVisible(hwnd))
	{
		char name[MAX_PATH + 1] = {};
		GetWindowTextA(hwnd, name, sizeof(name));

		if (name[0] != '\0')
		{
			ProcessWindow p;
			p.pid = pid;
			p.windowName = name;
			p.processName = "---notFound?---";

			allWindows.push_back(p);
		}
	}

	return true;
}

std::vector<ProcessWindow> getAllWindows()
{
	allWindows.clear();

	EnumWindows(EnumWindowsProc, 0);
	setNameOfProcesses();

	return allWindows;
}


PROCESS queriedProcess = 0;
char *baseQueriedPtr = (char *)0x0;

bool initVirtualQuery(PROCESS process)
{
	queriedProcess = process;
	baseQueriedPtr = 0;
	return 1;
}

bool getNextQuery(void **low, void **hi)
{
	MEMORY_BASIC_INFORMATION memInfo;

	bool rez = 0;
	while(true)
	{
		rez = VirtualQueryEx(queriedProcess, (void *)baseQueriedPtr, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));

		if (!rez) { return false; }

		if (memInfo.State == MEM_COMMIT && memInfo.Protect == PAGE_READWRITE)
		{
			//good page
			*low = memInfo.BaseAddress;
			*hi = (char *)memInfo.BaseAddress + memInfo.RegionSize;
			return true;
		}

	}
}



//http://kylehalladay.com/blog/2020/05/20/Rendering-With-Notepad.html
std::vector<void*> findBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen)
{
	if (patternLen == 0) { return {}; }

	std::vector<void*> returnVec;
	returnVec.reserve(1000);

	char* basePtr = (char*)0x0;

	MEMORY_BASIC_INFORMATION memInfo;

	while (VirtualQueryEx(process, (void*)basePtr, &memInfo, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		
		if (memInfo.State == MEM_COMMIT && memInfo.Protect == PAGE_READWRITE)
		{
			//search for our patern
			char* remoteMemRegionPtr = (char*)memInfo.BaseAddress;
			char* localCopyContents = new char[memInfo.RegionSize];

			SIZE_T bytesRead = 0;
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
		}

		basePtr = (char*)memInfo.BaseAddress + memInfo.RegionSize;
	}

	return returnVec;
}

PROCESS openProcessFromPid(PID pid)
{

	HANDLE handleToProcess = OpenProcess(
		PROCESS_VM_READ |
		PROCESS_QUERY_INFORMATION |
		PROCESS_VM_WRITE |
		PROCESS_VM_OPERATION, 0, pid);

	if ((handleToProcess == INVALID_HANDLE_VALUE) || (handleToProcess == 0))
	{
		handleToProcess = 0;
	}

	return handleToProcess;
}

void closeProcess(PROCESS process)
{
	CloseHandle(process);
}


#endif
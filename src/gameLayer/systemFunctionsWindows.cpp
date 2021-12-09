//http://kylehalladay.com/blog/2020/05/20/Hooking-Input-Snake-In-Notepad.html
//https://github.com/milkdevil/injectAllTheThings

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

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


std::string printAllProcesses(PID& pid)
{
	ImGui::PushID(30000);
	HANDLE h;
	PROCESSENTRY32 singleProcess = {sizeof(PROCESSENTRY32)};
	h = CreateToolhelp32Snapshot( //takes a snapshot of specified processes
		TH32CS_SNAPPROCESS, //get all processes
		0); //ignored for SNAPPROCESS

	static std::vector<char*> processesNames;
	static DWORD lastPid = 0;

	processesNames.reserve(500); //todo fix later
	for (auto& i : processesNames)
	{
		i[0] = '\0';
	}

	int index = 0;
	static int itemCurrent = 0;
	static int lastItemCurrent = -1;
	bool found = 0;

	while (Process32Next(h, &singleProcess))
	{
		if (singleProcess.th32ProcessID == 0) { continue; } //ignore system process

		if (index >= processesNames.size())
		{
			processesNames.push_back(new char[MAX_PATH]);
		}

		strcpy(processesNames[index], singleProcess.szExeFile);
		
		if (lastItemCurrent != itemCurrent)
		{
			if (itemCurrent == index)
			{
				lastPid = singleProcess.th32ProcessID;
				found = true;
			}
		}
		else
		if (lastPid)
		{
			if (singleProcess.th32ProcessID == lastPid)
			{
				itemCurrent = index;
				found = true;
			}
		}

		index++;
	} 

	if (found == false)
	{
		itemCurrent = 0;
		lastPid = 0;
		lastItemCurrent = -1;
	}
	else
	{
		lastItemCurrent = itemCurrent;
	}

	CloseHandle(h);

	ImGui::Combo("##processes list box", &itemCurrent, &processesNames[0], index);

	ImGui::PopID();

	pid = lastPid;
	if (found)
	{
		//return "";
		return std::string(processesNames[itemCurrent]);
	}
	else
	{
		return "";
	}
}


std::vector<HWND> windows;
std::vector<std::string> windowsPidsNames;
std::vector<std::string> windowsNames;
std::vector<DWORD> pids;

void setNameOfProcesses()
{
	HANDLE h;
	PROCESSENTRY32 singleProcess = {sizeof(PROCESSENTRY32)};
	h = CreateToolhelp32Snapshot( //takes a snapshot of specified processes
		TH32CS_SNAPPROCESS, //get all processes
		0); //ignored for SNAPPROCESS

	windowsPidsNames.clear();
	windowsPidsNames.resize(windows.size(), "---notFound---");

	while (Process32Next(h, &singleProcess))
	{
		auto name = singleProcess.szExeFile;
		auto pid = singleProcess.th32ProcessID;

		auto f = std::find(pids.begin(), pids.end(), pid);
	
		if (f != pids.end())
		{
			int index = f - pids.begin();

			windowsPidsNames[index] = name;

		}
		

	}
	
	for (int i = 0; i < windowsPidsNames.size(); i++)
	{
		if (windowsPidsNames[i] == "---notFound---")
		{
			windows.erase(windows.begin() + i);
			windowsPidsNames.erase(windowsPidsNames.begin() + i);
			pids.erase(pids.begin() + i);
			windowsNames.erase(windowsNames.begin() + i);
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
			windows.push_back(hwnd);
			pids.push_back(pid);
			windowsNames.push_back(name);
		}
	}

	return true;
}

std::string printAllWindows(PID& pid)
{
	ImGui::PushID(20000);

	windows.clear();
	pids.clear();
	windowsNames.clear();
	EnumWindows(EnumWindowsProc, 0);

	static std::vector<char*> processesNames;
	static DWORD lastPid = 0;

	processesNames.reserve(500); //todo fix later
	for (auto& i : processesNames)
	{
		i[0] = '\0';
	}

	static int itemCurrent = -1;
	static int lastItemCurrent = 0;
	bool found = 0;
	int index = 0;

	setNameOfProcesses();

	for(index = 0; index < windows.size(); index++)
	{

		if (index >= processesNames.size())
		{
			processesNames.push_back(new char[MAX_PATH]);
		}

		strcpy(processesNames[index], windowsNames[index].c_str()); //todo

		DWORD pid = pids[index];

		if (lastItemCurrent != itemCurrent)
		{
			if (itemCurrent == index)
			{
				lastPid = pid;
				found = true;
			}
		}
		else
		if (lastPid)
		{
			if (pid == lastPid)
			{
				itemCurrent = index;
				found = true;
			}
		}

	}

	if (found == false)
	{
		itemCurrent = 0;
		lastPid = 0;
		lastItemCurrent = -1;
	}
	else
	{
		lastItemCurrent = itemCurrent;
	}

	ImGui::Combo("##windows list box", &itemCurrent, &processesNames[0], index);

	ImGui::PopID();

	pid = lastPid;
	if (found)
	{
		return windowsPidsNames[itemCurrent];
	}
	else
	{
		return "";
	}

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


void refindBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen, std::vector<void*> &found)
{
	if (patternLen == 0) { return; }
	
	std::vector<void*> newFound;
	newFound.reserve(found.size());

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
						newFound.push_back((char*)memInfo.BaseAddress + curPos);
					}

					curPos++;
					cur++;
				}


			}

			delete[] localCopyContents;
		}

		basePtr = (char*)memInfo.BaseAddress + memInfo.RegionSize;
	}

	std::vector<void*> intersect;
	intersect.resize(std::min(found.size(), newFound.size()));
	
	std::set_intersection(found.begin(), found.end(),
		newFound.begin(), newFound.end(),
		intersect.begin());

	intersect.erase(std::remove(intersect.begin(), intersect.end(), nullptr), intersect.end());

	found = std::move(intersect);

}

#endif
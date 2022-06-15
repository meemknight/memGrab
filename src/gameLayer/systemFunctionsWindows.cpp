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
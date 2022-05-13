#include "systemFunctions.h"
#include "imgui.h"
#include <vector>
#include <algorithm>

#undef min
#undef max

std::string printAllProcesses(PID& pid)
{
	ImGui::PushID(30000);

	auto processes = getAllProcesses();

	static PID lastPid = 0;

	int index = 0;
	static int itemCurrent = 0;
	static int lastItemCurrent = -1;
	bool found = 0;

	for (index = 0; index < processes.size(); index++)
	{

		if (lastItemCurrent != itemCurrent)
		{
			if (itemCurrent == index)
			{
				lastPid = processes[index].second;
				found = true;
			}
		}
		else
			if (lastPid)
			{
				if (processes[index].second == lastPid)
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

	ImGui::Combo("##processes list box", &itemCurrent,
		[](void* data, int idx, const char** out_text)
	{
		*out_text = (*((const std::vector<std::pair<std::string, PID>>*)data))[idx].first.c_str(); return true;
	}
		,
		&processes, processes.size());

	ImGui::PopID();

	pid = lastPid;
	if (found)
	{
		return processes[itemCurrent].first;
	}
	else
	{
		return "";
	}
}

std::string printAllWindows(PID& pid)
{
	ImGui::PushID(20000);

	auto windows = getAllWindows();

	static std::vector<char*> processesNames;
	static PID lastPid = 0;

	processesNames.reserve(500); //todo fix later
	for (auto& i : processesNames)
	{
		i[0] = '\0';
	}

	static int itemCurrent = -1;
	static int lastItemCurrent = 0;
	bool found = 0;
	int index = 0;

	for (index = 0; index < windows.size(); index++)
	{

		if (index >= processesNames.size())
		{
			processesNames.push_back(new char[MAX_PATH_COMMON]);
		}

		strcpy(processesNames[index], windows[index].windowName.c_str()); //todo

		PID pid = windows[index].pid;

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
		return windows[itemCurrent].processName;
	}
	else
	{
		return "";
	}

}

void refindBytePatternInProcessMemory(PROCESS process, void* pattern, size_t patternLen, std::vector<void*>& found)
{
	if (patternLen == 0) { return; }

	auto newFound = findBytePatternInProcessMemory(process, pattern, patternLen);

	std::vector<void*> intersect;
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
		if ((flags | memQueryFlags_Read) && (flags | memQueryFlags_Write))
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
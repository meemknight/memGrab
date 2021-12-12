#include "systemFunctions.h"
#include "imgui.h"
#include <vector>
#include <algorithm>

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

	for (index = 0; index < windows.size(); index++)
	{

		if (index >= processesNames.size())
		{
			processesNames.push_back(new char[MAX_PATH]);
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

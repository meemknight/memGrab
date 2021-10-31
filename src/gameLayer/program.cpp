#include "program.h"
#include "imgui.h"
#include "systemFunctions.h"
#include <iostream>
#include <sstream>

#undef min
#undef max


void OpenProgram::render()
{


	//ImGui::Text("Process name");
	//bool open = ImGui::InputText("##Enter process name", processName, sizeof(processName), ImGuiInputTextFlags_EnterReturnsTrue);

	if (ImGui::BeginTabBar("##open selector"))
	{
		DWORD selectedP = 0;
		std::string processName = "";
		bool open = 0;

		if (ImGui::BeginTabItem("process name"))
		{
			processName = printAllProcesses(selectedP);
			if (ImGui::Button("open"))
			{
				open = true;
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("window"))
		{
			processName = printAllWindows(selectedP);
			if (ImGui::Button("open"))
			{
				open = true;
			}
			ImGui::EndTabItem();
		}

		if (open)
		{
			fileOpenLog.clearError();

			//pid = findPidByName(processName);z
			pid = selectedP;

			if (pid == 0)
			{
				fileOpenLog.setError("No process selected");
			}
			else
			{
				strcpy(currentPocessName, processName.c_str());
				handleToProcess = OpenProcess(
					PROCESS_VM_READ |
					PROCESS_QUERY_INFORMATION |
					PROCESS_VM_WRITE |
					PROCESS_VM_OPERATION, 0, pid);

				if ((handleToProcess == INVALID_HANDLE_VALUE) || (handleToProcess == 0))
				{
					std::string s = "couldn't open process\n";
					s += getLastErrorString();
					fileOpenLog.setError(s.c_str());
					handleToProcess = 0;
					pid = 0;
				}
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::Separator();

}


void SearchForValue::clear()
{
	foundValues.clear();
	currentItem = 0;
	data = {};
	str = {};
}

void *SearchForValue::render(HANDLE handle)
{
	ImGui::PushID(handle);

	ImGui::Text("search for value");

	bool changed = 0;
	typeInput<__COUNTER__ + 1000>(data, 0, &changed, &str);
	
	void* foundPtr = 0;

	if (changed)
	{
		foundValues.clear();
	}

	if (ImGui::Button("search"))
	{
		if (foundValues.empty())
		{
			if (data.type == Types::t_string)
			{
				foundValues = findBytePatternInProcessMemory(handle, (void*)str.c_str(), str.length());
			}
			else
			{
				foundValues = findBytePatternInProcessMemory(handle, data.ptr(), data.getBytesSize());
			}
		}
		else
		{
			if (data.type == Types::t_string)
			{
				refindBytePatternInProcessMemory(handle, (void*)str.c_str(), str.length(), foundValues);
			}
			else
			{
				refindBytePatternInProcessMemory(handle, data.ptr(), data.getBytesSize(), foundValues);
			}

		}

	}
	ImGui::SameLine();
	if (ImGui::Button("clearSearch"))
	{
		foundValues.clear();
	}
	
	if (!foundValues.empty())
	{
		for (int i = 0; i < foundValues.size(); i++)
		{
			if (i >= foundValuesText.size())
			{
				foundValuesText.push_back(new char[17]);
			}

			std::stringstream ss;
			ss << std::hex << ((unsigned long long)foundValues[i]);

			strcpy(foundValuesText[i], ss.str().c_str());

		}


		ImGui::Text("Found pointers: %d", foundValues.size());
		ImGui::ListBox("##found pointers", &currentItem, &foundValuesText[0], foundValues.size(), std::min(10ull, foundValues.size()));
		
		if (currentItem < foundValues.size())
		{
			foundPtr = foundValues[currentItem];
		}

		if (foundPtr)
		{
			if (ImGui::Button(std::string("Copy: " + std::string(foundValuesText[currentItem])).c_str()))
			{

			}else
			{
				foundPtr = 0;
			}
		}

	}

	ImGui::PopID();

	return foundPtr;
}

void OppenedProgram::render()
{
	

	if (pid != 0)
	{
		ImGui::PushID(pid);
		//ImGui::Begin(currentPocessName);


		ImGui::Text("Process id: %d, name: %s", pid, currentPocessName);
		static int v = 0;
		ImGui::NewLine();

		//ImGui::Text("Search for a value");
		//typeInput<__COUNTER__>();

		static void* memLocation = {};
		ImGui::NewLine();
		ImGui::Text("Enter memory location");
		ImGui::InputScalar("##mem location", ImGuiDataType_U64, &memLocation, 0, 0, "%016" IM_PRIx64, ImGuiInputTextFlags_CharsHexadecimal);
		ImGui::NewLine();

	#pragma region write to memory
		{
			ImGui::Text("Write to memory");
			bool enterPressed = 0;
			static std::string s;
			static GenericType data;
			typeInput<__COUNTER__>(data, &enterPressed, 0, &s);
			ImGui::SameLine();
			enterPressed |= ImGui::Button("Write");

			if (enterPressed)
			{
				if (data.type == Types::t_string)
				{
					writeMemory(handleToProcess, (void*)memLocation, (void*)s.c_str(), s.size(), writeLog);
				}
				else
				{
					writeMemory(handleToProcess, (void*)memLocation, data.ptr(), data.getBytesSize(), writeLog);
				}
			}

			writeLog.renderText();

			ImGui::NewLine();
		}
	#pragma endregion

		auto searched = searcher.render(handleToProcess);

		if (searched)
		{
			memLocation = searched;
		}


		//ImGui::End();
		ImGui::PopID();
	}

}

bool OppenedProgram::isAlieve()
{
	if (!pid) { return true; }

	if (isProcessAlive(handleToProcess))
	{
		return true;
	}
	else
	{
		return false;
	}
}

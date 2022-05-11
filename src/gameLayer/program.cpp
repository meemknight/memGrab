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
		PID selectedP = 0;
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
				handleToProcess = openProcessFromPid(pid);

				if (handleToProcess == 0)
				{
					std::string s = "couldn't open process\n";
					s += getLastErrorString();
					fileOpenLog.setError(s.c_str());
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

void *SearchForValue::render(PROCESS handle)
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


		ImGui::Text("Found pointers: %d", (int)foundValues.size());
		ImGui::ListBox("##found pointers", &currentItem, &foundValuesText[0], foundValues.size(), std::min((decltype(foundValues.size()))10ull, foundValues.size()));
		
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

bool OppenedProgram::render()
{
	std::stringstream s;
	s << "Process: ";
	s << currentPocessName << "##" << pid << "open and use process";
	bool oppened = 1;
	if (ImGui::Begin(s.str().c_str(), &oppened, ImGuiWindowFlags_NoSavedSettings))
	{

		if (pid != 0 && isOppened)
		{
			ImGui::PushID(pid);
			//ImGui::Begin(currentPocessName);

			ImGui::Text("Process id: %d, name: %s", (int)pid, currentPocessName);
			static int v = 0;
			ImGui::NewLine();

			//ImGui::Text("Search for a value");
			//typeInput<__COUNTER__>();

			static void *memLocation = {};
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
						writeMemory(handleToProcess, (void *)memLocation, (void *)s.c_str(), s.size(), writeLog);
					}
					else
					{
						writeMemory(handleToProcess, (void *)memLocation, data.ptr(), data.getBytesSize(), writeLog);
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
		else
		{
			ImGui::Text("Process name: %s", currentPocessName);
		}

		errorLog.renderText();

	}

	ImGui::End();

	s << "Hex";

	if (ImGui::Begin(s.str().c_str(), &oppened, ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::PushID(pid);
		if (pid != 0 && isOppened)
		{
			ImGui::Text("hex editor goes here");

			ImGui::BeginGroup();


			for (int i = 0; i < 10; i++)
			{

				for (int i = 0; i < 16; i++)
				{
					ImGui::Text("%c%c ", 'a' + i, 'a' + i);
					if (i < 15) { ImGui::SameLine(); }
				}

			}

			void *ptr;

			hexLog.setError("warn", ErrorLog::ErrorType::warn);

			ImGui::EndGroup();


		}
		else
		{
			ImGui::Text("Process name: %s", currentPocessName);
		}

		hexLog.renderText();

		ImGui::NewLine();

		ImGui::PopID();
	}
	
	ImGui::End();

	return oppened;
}

void OppenedProgram::close()
{
	closeProcess(handleToProcess);
	isOppened = 0;
	handleToProcess = 0;
	searcher.clear();
}

bool OppenedProgram::isAlieve()
{
	if (!pid) { return false; }
	if (!isOppened) { return false; }

	if (isProcessAlive(handleToProcess))
	{
		return true;
	}
	else
	{
		return false;
	}
}

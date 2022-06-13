#include "program.h"
#include "imgui.h"
#include "systemFunctions.h"
#include <iostream>
#include <sstream>
#include <imgui_internal.h>

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




struct Sizes {


	unsigned int cols = 16;
	float hexCharWidth = ImGui::CalcTextSize("F").x + 1; // width of one hexChar
	float hexCellWidth; // width of the inputText -> used for highlight and mouseCapture in the example i think

};


//  will use it to add generals styles for the hexEditor; dont know exactly where in the code it goes :))
//  probably anywhere between the 2 ImGui::Begin()
//  maybe save styles and the undo style changes to not influence further windows
void AddStyles()
{
	ImGuiStyle& style = ImGui::GetStyle();

	
}


enum boxColor {
	boxColorCommitted = 0xaf000000, /// Alpha, Blue, Green, Red
	boxColorUncommitted = 0xff333333, /// Alpha, Blue, Green, Red
	boxColorRead = 0xbb4545ba, // red for readable memory
	boxColorReadWrite = 0xcc089500, // green for readwrite memory
};

enum textColor {
	txtColorCommitted = 0xffffffff, /// Alpha, Blue, Green, Red
	txtColorUncommitted = 0xffffffff, //
	txtColorRead = 0xffffffff, // 
	txtColorReadWrite = 0xffffffff, //
};

int getBitsFromHexChar(char c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('A' <= c && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

void OppenedProgram::drawHexes(void* memData, void* memFlags, size_t memSize)
{
	ImGui::SetWindowFontScale(1.f);
	Sizes sizes;
	size_t i, j;
	char buf[33];
	char asciiBuf[17];
	const char* addressFormat = "%#0*zX";
	const char* byteFormat = "%02X";

	if (sizes.cols > 16) // buf only has space for 32 chars which can only display 16 bytes in hex
		sizes.cols = 16;
	
	size_t lines = (memSize + sizes.cols - 1) / sizes.cols;
	size_t addrDigits = 0;
	size_t lastAddr = memSize + memoryAddress - 1; // use the largest address to compute digits needed for display

	while (lastAddr > 0)
	{
		++addrDigits;
		lastAddr >>= 4;
	}
	addrDigits += 2; // add 2 more spaces for the 0X at the beginning
	ImU8* memory = (ImU8*)memData; // byte pointer; used to copy bytes
	char* memoryFlags = (char*)memFlags;

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

	if (ImGui::BeginChild("##Data", {0.f, ImGui::GetWindowHeight()*0.7f}, true))
	{
		for (i = 0; i < lines; ++i)
		{
			size_t addr = i * sizes.cols;
			ImGui::Text(addressFormat, addrDigits, addr + memoryAddress);
			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

			for (j = 0; j < sizes.cols && addr + j < memSize; ++j)
			{
				ImGui::SameLine();
				ImGui::PushID(sizes.cols * i + j); // push a hexCell ID

				// set general flags for every situation
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_AlwaysOverwrite |
					ImGuiInputTextFlags_AutoSelectAll;
				
				if (memoryFlags[addr + j] & memQueryFlags_Write)
				{
					sprintf(buf, byteFormat, memory[addr + j]);
					flags |= ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorReadWrite));
					ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorReadWrite));

					ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 5);
					if (ImGui::InputText("##hexCell", buf, 3, flags))
					{
						if (buf[0] != 0 && buf[1] != 0)
						{
							char c = (getBitsFromHexChar(buf[0]) << 4) + getBitsFromHexChar(buf[1]);
							writeMemory(handleToProcess, (void*) (memoryAddress + addr + j), (void*) &c, (size_t)1, writeLog);
						}
					}

					ImGui::PopStyleColor();
					ImGui::PopStyleColor();
				}
				else if (memoryFlags[addr + j] & memQueryFlags_Read)
				{
					sprintf(buf, byteFormat, memory[addr + j]);
					ImGui::Text(" %c%c", buf[0], buf[1]);
				}
				else if (memoryFlags[addr + j] & memQueryFlags_Comitted)
				{
					int x = '?';
					memset(buf, x, 33);
					flags |= ImGuiInputTextFlags_ReadOnly;
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorCommitted));
					ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorCommitted));

					ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 5);
					ImGui::InputText("##hexCell", buf, 3, flags);

					ImGui::PopStyleColor();
					ImGui::PopStyleColor();
				}
				else // uncommitted memory
				{
					int x = '?';
					memset(buf, x, 33);
					flags |= ImGuiInputTextFlags_ReadOnly;
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorUncommitted));
					ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorUncommitted));

					ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 5);
					ImGui::InputText("##hexCell", buf, 3, flags);

					ImGui::PopStyleColor();
					ImGui::PopStyleColor();
				}
				
				ImGui::PopID();
			}
			ImGui::SameLine();
			
			memset(buf, '?', 33);
			for (size_t dif = j; dif < sizes.cols; ++dif) // this only fills the last line if memSize not divisible by nrCols
			{
				ImGui::PushID(sizes.cols * i + j); // push a hexCell ID
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32(0xff42f2f5)); /// Alpha, Blue, Green, Red
				ImGui::PushStyleColor(ImGuiCol_Text, ImU32(0xff000000));

				ImGuiInputTextFlags flags = ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_ReadOnly; 			

				ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 5);
				std::cout << "asdads";
				ImGui::InputText("##hexCell", buf, 3, flags);

				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				ImGui::PopID();
				ImGui::SameLine();
			}
			
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			memset(asciiBuf, 0, 17);
			memcpy(asciiBuf, (char*)memData + addr, std::min((int)sizes.cols, (int)(memSize - addr)));
			for (int poz = 0; poz < 16; ++poz)
			{
				if (asciiBuf[poz] < 32 || asciiBuf[poz] > 126)
					asciiBuf[poz] = '.' ;
			}

			ImGui::Text("%s", asciiBuf);

		}
		if (scrollTopBot)
		{
			if (scrollTopBot == 1) // scroll up
			{
				ImGui::SetScrollY(0.f);
			}
			else
			{ 
				ImGui::SetScrollY(ImGui::GetTextLineHeightWithSpacing()*lines *2);
			}
			scrollTopBot = 0;
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
	

	ImGui::BeginChild("##Options", {0.f, 0.f}, true);
	{
		ImGui::BeginChild("##Navigate", {ImGui::GetWindowWidth()/2, 0.f}, true);
		{
			if (ImGui::Button("<", ImVec2(sizes.hexCharWidth + 6, 0)))
			{
				if (memoryAddress >= memSize - sizes.cols * 5)
					memoryAddress -= (memSize - sizes.cols*5);
				else
					memoryAddress = 0;
				scrollTopBot = -1;
			}
			
			ImGui::SameLine();
			ImGui::InputScalar("##BaseAddressForHex", ImGuiDataType_U64, &memoryAddress, 0, 0,
				"%016zX" , ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
			ImGui::SameLine();
			
			if (ImGui::Button(">", ImVec2(sizes.hexCharWidth + 6, 0)))
			{
				if (memoryAddress <= ((size_t)-1)  - 2*memSize + 1 + sizes.cols * 5)
					memoryAddress += (memSize - sizes.cols * 5);
				else
					memoryAddress = ((size_t) -1) - memSize + 1;
				scrollTopBot = 1;
			}
		}
		ImGui::EndChild();
		
		ImGui::SameLine();

		ImGui::BeginChild("##Notations", { ImGui::GetWindowWidth() / 2, 0.f }, true);
		{
			char common[5];
			int filler = '?';
			memset(common, filler, 4);
			ImGuiInputTextFlags commonFlags =  ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_ReadOnly;
			// ReadWrite
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorReadWrite));
			ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorReadWrite));

			ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 5);
			ImGui::InputText("##RWexample", common, 3, commonFlags);
			ImGui::SameLine();
			ImGui::Text("ReadWrite memory");

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			// ReadOnly
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorRead));
			ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorRead));

			ImGui::Text(" %c%c", buf[0], buf[1]);
			ImGui::SameLine();
			ImGui::Text("ReadOnly memory");

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();


			// Committed
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorCommitted));
			ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorCommitted));

			ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 5);
			ImGui::InputText("##COMMexample", common, 3, commonFlags);
			ImGui::SameLine();
			ImGui::Text("Committed memory");

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			// Uncommitted
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorUncommitted));
			ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorUncommitted));

			ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 5);
			ImGui::InputText("##UNCOMMexample", common, 3, commonFlags);
			ImGui::SameLine();
			ImGui::Text("Uncommitted memory");

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

		}
		ImGui::EndChild();

	}
	ImGui::EndChild();
	
	
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
	
	
	//ImGui::ShowDemoWindow();
	// hex editor
	ImGui::SetNextWindowSize(ImVec2(1000, 800));
	if (ImGui::Begin(s.str().c_str(), &oppened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::PushID(pid);
		if (pid != 0 && isOppened)
		{
			const size_t memSize = 4000;
			char buffer[memSize] = {};
			char memoryFlags[memSize] = {};
			void *low, *high;
			int flags = 0;

			OppenedQuery query = initVirtualQuery(handleToProcess);
			
			// keep this for the bug report -> overflow when memoryAddress is (size_t) -1
			// memoryAddress = std::min(memoryAddress, ((size_t)-1) - memSize + 1);

			while (getNextQuery(query, low, high, flags)) // set memory flags;
			{

				if (memoryAddress < (size_t)high) //catch memory ranges that start before memoryAddress;
				{
					if ((size_t)low >= memoryAddress + memSize) // out of range; skip it;
						continue;
					
					void* copyStart = (void*) std::max(memoryAddress, (size_t)low);
					void* copyEnd = (void*)std::min(memoryAddress + memSize, (size_t)high);
					// if (size_t)copyEnd < (size_t) copyStart -> overflow took place; will be fixed afterwards so me can make the bug report

					size_t bytesToCopy = (size_t)copyEnd - (size_t)copyStart;

					if (flags & memQueryFlags_Read)
					{
						readMemory(handleToProcess, copyStart, bytesToCopy, buffer + ((size_t)copyStart - memoryAddress));					
					}
					memset(memoryFlags + ((size_t)copyStart - memoryAddress), flags, bytesToCopy);
				}
			}
			
			drawHexes(buffer, memoryFlags, memSize);
			
			// void* ptr;
			// hexLog.setError("warn", ErrorLog::ErrorType::warn);

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

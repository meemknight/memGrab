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


// compute nr of lines
// display first address on each line
// second child should have the buffer in clear ascii
// add *flags



void drawHexes(void* memData, void* memFlags, size_t memSize, Sizes& sizes, size_t& baseAddr, int& pageNumber) // remove pageNr later
{
	size_t i, j;
	char buf[33];
	char asciiBuf[17];
	const char* addressFormat = "%#0*X";
	const char* byteFormat = "%02X";

	if (sizes.cols > 16) // buf only has space for 32 chars which can only display 16 bytes in hex
		sizes.cols = 16;
	
	size_t lines = (memSize + sizes.cols - 1) / sizes.cols;
	size_t addrDigits = 0;
	size_t lastAddr = memSize + baseAddr - 1; // use the largest address to compute digits needed for display

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
			ImGui::Text(addressFormat, addrDigits, addr + baseAddr);
			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

			for (j = 0; j < sizes.cols && addr + j < memSize; ++j)
			{
				ImGui::SameLine();
				ImGui::PushID(sizes.cols * i + j); // push a hexCell ID

				// set general flags for every situation
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_AlwaysOverwrite |
					ImGuiInputTextFlags_AutoSelectAll;

				// if it has write flag it also has the read flag
				if (memoryFlags[addr + j] & memQueryFlags_Write) 
					flags |= ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;	
				else if(memoryFlags[addr + j] & memQueryFlags_Read) // it has read so there is readable memory in buffer
					flags |= ImGuiInputTextFlags_ReadOnly;
				else if(memoryFlags[addr = j] & memQueryFlags_Execute) // no read/write -> PAGE_EXECUTE;   0;
					flags |= ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_Password; // there is actual memory there but it can't be read; display differently from no memory
				else // no flags are set -> no memory push styles?
				{
					int x = '?';
					memset(buf, x, 33);
				}

				// as for showing, buf already something copied; either 0s or machine code
				// for now will leave it as 00 for no memmory; or just make an if ; no flags means overwrite buf with ??

				sprintf(buf, byteFormat, memory[addr + j]);
				if (memoryFlags[addr + j] & memQueryFlags_Execute)
				{
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32(0xffffffff)); /// Alpha, Blue, Green, Red
					ImGui::PushStyleColor(ImGuiCol_Text, ImU32(0xff000000));
				}
				ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 4);
					
				ImGui::InputText("##hexCell", buf, 3, flags);
				
				if (memoryFlags[addr + j] & memQueryFlags_Execute)
				{
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
				// add readOnly since it's not actual data				

				ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 4);

				if (ImGui::InputText("##hexCell", buf, 3, flags))
				{
					// try to copy user data to memory; to add
				}

				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				ImGui::PopID();
				ImGui::SameLine();
			}
			
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			memset(asciiBuf, 0, 17);
			memcpy(asciiBuf, (char*)memData + addr, std::min((int)sizes.cols, (int)(memSize - addr)));
			ImGui::Text("%s", asciiBuf);

		}
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
	

	// to complete later 
	ImGui::BeginChild("##Options", {}, true);
	{
		ImGui::InputScalar("##BaseAddressForHex", ImGuiDataType_U64, &baseAddr, 0, 0, 0);
		if (ImGui::Button("nextPage", ImVec2(100, 40)))
		{
			++pageNumber;
			baseAddr = 0;
			if (pageNumber > 500) // add random reset for now; found 680 pages in one run; leave it like so for now
				pageNumber = 0;
		}
		if (ImGui::Button("previousPage", ImVec2(100, 40)))
		{
			--pageNumber;
			baseAddr = 0;
			if (pageNumber < 0)
				pageNumber = 0;
		}
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
	
	//ImGui::SetNextWindowSize(ImVec2(800, 600));
	if (ImGui::Begin(s.str().c_str(), &oppened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::PushID(pid);
		if (pid != 0 && isOppened)
		{
			// ImGui::ShowDemoWindow();
			// char testData[100] = "Throw some random chars in here";
			// size_t testSize = 16;

			// size_t memoryAddress -> use for nextQuery
			const size_t memSize = 4000;
			char buffer[memSize] = {};
			char memoryFlags[memSize] = {}; // could use memset to put flag on certain range
			void* low, *high;
			int flags = 0;
			OppenedQuery query = initVirtualQuery(handleToProcess);
			int pageFound = 0; // found >650 pages in one test
			while (getNextQuery(query, low, high, flags)) // set memory flags;
			{
				// temporary to find some data since it the address range is really large;
				// printed first addres and it was a 15 digit number while we work with 4000 bytes ;
				// GL finding addresses that have data searching blindly :)))
				++pageFound;
				if (memoryAddress == 0 && pageFound == this->page) // change page in program.h
					memoryAddress = (size_t)low;

				if ((size_t)low >= (size_t)memoryAddress && (size_t)low <= (size_t)memoryAddress + memSize)
				{
					size_t bytesToCopy = std::min((memoryAddress + memSize) - (size_t)low, (size_t)high - (size_t)low);
					// (baseAddr+memSize - low) -> nr of bytes from low to end of buffer -> available space
					// (high - low) -> actual size of memory -> desired space to read the entire page
					
					//if (flags)
					//	std::cout << "ASDASD\n";
					//else
					//	std::cout << "DSADSA\n";

					// only read if allowed
					if(flags & memQueryFlags_Read)
						readMemory(handleToProcess, low, bytesToCopy, buffer + (memoryAddress - (size_t)low));
					
					if (flags & memQueryFlags_Execute) // this found no pages...; idk couldn't test everything unfortunately
					{									// if u see things in cmd it could be because of this; 10/10
						if (this->page != 0)
						{
							this->page = 0;
							this->memoryAddress = (size_t)low;
						}
						std::cout << "reached\n";
					}
					// always save flags for the hexEditor
					memset(memoryFlags + (memoryAddress - (size_t)low), flags, bytesToCopy); // set memory flags for that range
				}
			}
			Sizes ss;
			drawHexes(buffer, memoryFlags, memSize, ss, memoryAddress, page);
			
			//ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase ))  | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_AlwaysOverwrite | ImGuiInputTextFlags_AutoSelectAll)) , hexCellCallback, &userData))


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

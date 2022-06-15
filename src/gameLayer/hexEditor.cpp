#include "hexEditor.h"
#include "imgui_internal.h"
#include "systemFunctions.h"
#include <algorithm>
#include "virtualQueryApi.h"

enum boxColor
{
	boxColorCommitted = 0xaf000000, /// Alpha, Blue, Green, Red
	boxColorUncommitted = 0xff333333, /// Alpha, Blue, Green, Red
	boxColorRead = 0xbb4545ba, // red for readable memory
	boxColorReadWrite = 0xcc089500, // green for readwrite memory
};

enum textColor
{
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


void HexEditor::drawHexes(void* memData, void* memFlags, size_t memSize, PROCESS handleToProcess)
{
	struct Sizes
	{
		unsigned int cols = 16;
		float hexCharWidth = ImGui::CalcTextSize("F").x + 1; // width of one hexChar
		float hexCellWidth = 0; // width of the inputText -> used for highlight and mouseCapture in the example i think
	};


	ImGui::SetWindowFontScale(1.f);
	Sizes sizes;
	float windowWitdh = ImGui::GetWindowWidth();
	
	int fittingCols = (windowWitdh - 230) / 34 + 1;
	fittingCols = std::max(1, fittingCols);
	sizes.cols = std::min(sizes.cols, (unsigned int) fittingCols);
	
	size_t i, j;
	char buf[33];
	char asciiBuf[17];
	const char* addressFormat = "%#016zX";
	const char* byteFormat = "%02X";

	if (sizes.cols > 16)
		sizes.cols = 16;
	
	size_t lines = memSize/ sizes.cols;

	ImU8* memory = (ImU8*)memData;
	char* memoryFlags = (char*)memFlags;

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

	if (ImGui::BeginChild("##Data", {0.f, ImGui::GetWindowHeight()*0.7f}, true))
	{
		for (i = 0; i < lines; ++i)
		{
			size_t addr = i * sizes.cols;
			ImGui::Text(addressFormat, addr + memoryAddress);
			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

			for (j = 0; j < sizes.cols && addr + j < memSize; ++j)
			{
				ImGui::SameLine();
				ImGui::PushID(sizes.cols * i + j);

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
							writeMemory(handleToProcess, (void*) (memoryAddress + addr + j), (void*) &c, (size_t)1, hexLog);
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
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			memset(asciiBuf, 0, 17);
			memcpy(asciiBuf, (char*)memData + addr, std::min((int)sizes.cols, (int)(memSize - addr)));
			for (int poz = 0; poz < 16; ++poz)
			{
				if (asciiBuf[poz] < 32 || asciiBuf[poz] > 126)
					asciiBuf[poz] = '.' ;
			}
			asciiBuf[sizes.cols] = 0; // add an early ending if the window is small
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

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::Text("ReadWrite memory");

			// ReadOnly
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorRead));
			ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorRead));

			ImGui::Text(" %c%c", buf[0], buf[1]);

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::Text("ReadOnly memory");


			// Committed
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorCommitted));
			ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorCommitted));

			ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 5);
			ImGui::InputText("##COMMexample", common, 3, commonFlags);

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::Text("Committed memory");

			// Uncommitted
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImU32((unsigned int)boxColorUncommitted));
			ImGui::PushStyleColor(ImGuiCol_Text, ImU32((unsigned int)txtColorUncommitted));

			ImGui::SetNextItemWidth(sizes.hexCharWidth * 2 + 5);
			ImGui::InputText("##UNCOMMexample", common, 3, commonFlags);

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::Text("Uncommitted memory");
		}
		ImGui::EndChild();

	}
	ImGui::EndChild();
	
	
}

void HexEditor::render(PROCESS handleToProcess, PID pid, const std::string &processName,
	const std::stringstream &processIdString)
{
	bool oppened = true;

	// hex editor
	ImGui::SetNextWindowSize(ImVec2(800, 700), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(processIdString.str().c_str(), &oppened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::PushID(pid);
		if (pid != 0)
		{
			const size_t memSize = 4000;
			char buffer[memSize] = {};
			char memoryFlags[memSize] = {};
			void *low, *high;
			int flags = 0;

			OppenedQuery query = initVirtualQuery(handleToProcess);

			memoryAddress = std::min(memoryAddress, ((size_t)-1) - memSize + 1);

			while (getNextQuery(query, low, high, flags)) // set memory flags;
			{

				if (memoryAddress < (size_t)high) //catch memory ranges that start before memoryAddress;
				{
					if ((size_t)low >= memoryAddress + memSize) // out of range; skip it;
						continue;

					void *copyStart = (void *)std::max(memoryAddress, (size_t)low);
					void *copyEnd = (void *)std::min(memoryAddress + memSize, (size_t)high);
					// if (size_t)copyEnd < (size_t) copyStart -> overflow took place; will be fixed afterwards so me can make the bug report

					size_t bytesToCopy = (size_t)copyEnd - (size_t)copyStart;

					if (flags & memQueryFlags_Read)
					{
						readMemory(handleToProcess, copyStart, bytesToCopy, buffer + ((size_t)copyStart - memoryAddress));
					}
					memset(memoryFlags + ((size_t)copyStart - memoryAddress), flags, bytesToCopy);
				}
			}

			drawHexes(buffer, memoryFlags, memSize, handleToProcess);

		}
		else
		{
			ImGui::Text("Process name: %s", processName);
		}

		hexLog.renderText();

		ImGui::NewLine();

		ImGui::PopID();
	}
	ImGui::End();

}

#include "gameLayer.h"
#include <glad/glad.h>
#include "platformInput.h"
#include "imgui.h"
#include "imguiThemes.h"
#include "systemFunctions.h"
#include "errorLogging.h"
#include "program.h"
#include <iostream>
#include <sstream>
#include "platformTools.h"
#include <algorithm>

struct GameData
{

}gameData;


bool initGame()
{

	if(!platform::readEntireFile(RESOURCES_PATH "gameData.data", &gameData, sizeof(GameData)))
	{
		gameData = GameData();
	}
	imguiThemes::green();

	return true;
}

bool gameLogic(float deltaTime)
{
#pragma region init stuff
	int w = 0; int h = 0;
	w= platform::getWindowSizeX();
	h = platform::getWindowSizeY();
	
	glClear(GL_COLOR_BUFFER_BIT);
#pragma endregion

	ImVec2 vWindowSize = ImGui::GetMainViewport()->Size;
	ImVec2 vPos0 = ImGui::GetMainViewport()->Pos;
	ImGui::SetNextWindowPos(ImVec2((float)vPos0.x, (float)vPos0.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)vWindowSize.x, (float)vWindowSize.y), 0);
	if (ImGui::Begin(
		"Editor",
		/*p_open=*/nullptr,
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoTitleBar
		)
		)
	{

		static const ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
		ImGuiID dockSpace = ImGui::GetID("MainWindowDockspace");
		ImGui::DockSpace(dockSpace, ImVec2(0.0f, 0.0f), dockspaceFlags);
		static char name[256] = {};

		static bool openWindow=1;

		const char *myData = "my data";

		//static OppenedProgram oppenedProgram;
		static OpenProgram openProgram;
		static std::vector<OppenedProgram> programsOppened;

		if (ImGui::BeginMenuBar())
		{

			if (ImGui::BeginMenu("Open..."))
			{

				openProgram.render();

				openProgram.fileOpenLog.renderText();
				
				ImGui::EndMenu();
			}


			ImGui::EndMenuBar();
		}


		if (openProgram.pid)
		{
			//first check if already oopened
			auto found = std::find_if(programsOppened.begin(), programsOppened.end(), [pid = openProgram.pid](const OppenedProgram& p)
			{
				return p.pid == pid;
			});

			if (found == programsOppened.end())
			{
				OppenedProgram newProgram;
				newProgram.isOppened = true;

				newProgram.pid = openProgram.pid;
				openProgram.pid = 0;

				newProgram.handleToProcess = openProgram.handleToProcess;
				openProgram.handleToProcess = 0;

				strcpy(newProgram.currentPocessName, openProgram.currentPocessName);
				openProgram.currentPocessName[0] = 0;

				programsOppened.push_back(newProgram);
			}
			else
			{
				openProgram.fileOpenLog.setError("Process already oppened.", ErrorLog::ErrorType::info);
				closeProcess(openProgram.handleToProcess);
				openProgram.pid = 0;
				openProgram.currentPocessName[0] = 0;
				openProgram.handleToProcess = 0;

			}
			
		}

		//if (oppenedProgram.pid)
		for(int i=0; i<programsOppened.size(); i++)
		{
			auto& program = programsOppened[i];

			if (!program.isAlieve())
			{
				//openProgram.fileOpenLog.setError("process closed", openProgram.fileOpenLog.info);
				program.writeLog.clearError();

				program.close();
				program.errorLog.setError("process closed", openProgram.fileOpenLog.info);
			}

			if(!program.render())
			{
				program.close();
				programsOppened.erase(programsOppened.begin() + i);
				i--;
			}
		}

		ImGui::End();
	}

	//ImGui::ShowDemoWindow();


#pragma region set finishing stuff

	return true;
#pragma endregion

}

const char *data = "hello worlddd\n";

void closeGame()
{

	platform::writeEntireFile(RESOURCES_PATH "gameData.data", &gameData, sizeof(GameData));

}

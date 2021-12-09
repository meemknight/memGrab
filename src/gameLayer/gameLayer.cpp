#include "gameLayer.h"
#include "gl2d/gl2d.h"
#include "platformInput.h"
#include "imgui.h"
#include "imguiThemes.h"
#include "systemFunctions.h"
#include "errorLogging.h"
#include "program.h"
#include <iostream>
#include <sstream>
#include "platformTools.h"

gl2d::Font font;
gl2d::Texture texture;

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

		static OppenedProgram oppenedProgram;
		static OpenProgram openProgram;

		if (ImGui::BeginMenuBar())
		{

			if (ImGui::BeginMenu("Open..."))
			{
				//ImGui::Checkbox("Open:", &openWindow);

				{

					//ImGui::Begin("Open##open and use process", &openWindow);

					openProgram.render();

					openProgram.fileOpenLog.renderText();

					//ImGui::End();

				}
				
				
				ImGui::EndMenu();
			}



			ImGui::EndMenuBar();
		}
		
		if (!(openProgram.pid == 0 && oppenedProgram.pid == 0))
		{

			if (openProgram.pid)
			{
				{
					oppenedProgram.pid = openProgram.pid;
					openProgram.pid = 0;

					oppenedProgram.handleToProcess = openProgram.handleToProcess;
					openProgram.handleToProcess = 0;

					strcpy(oppenedProgram.currentPocessName, openProgram.currentPocessName);
					openProgram.currentPocessName[0] = 0;

				}
			}

			if (oppenedProgram.pid)
			{
				std::stringstream s;
				s << "Process: ";
				s << oppenedProgram.pid;
				s << "##open and use process";
				ImGui::Begin(s.str().c_str());

				if (!oppenedProgram.isAlieve())
				{
					openProgram.fileOpenLog.setError("process closed", openProgram.fileOpenLog.info);
					oppenedProgram.writeLog.clearError();

					oppenedProgram.pid = 0;
					oppenedProgram.handleToProcess = 0;
					oppenedProgram.currentPocessName[0] = 0;
				}

				oppenedProgram.render();

				ImGui::End();
			}

		}


		ImGui::End();
	}


	//ImGui::ShowDemoWindow();


#pragma region set finishing stuff

	return true;
#pragma endregion

}

void closeGame()
{

	platform::writeEntireFile(RESOURCES_PATH "gameData.data", &gameData, sizeof(GameData));

}

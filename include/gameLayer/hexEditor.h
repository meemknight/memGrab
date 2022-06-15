#pragma once
#include "imgui.h"
#include <cstdint>
#include <string>
#include <vector>
#include "errorLogging.h"

struct HexEditor
{

	void render(PROCESS handleToProcess, PID pid, const std::string &processName, 
		const std::stringstream &processIdString);

	ErrorLog hexLog = {};
	size_t memoryAddress = 0;
	int scrollTopBot = 0; // 1 = up; 0 = nothing; -1 = botom

	void drawHexes(void *memData, void *memFlags, size_t memSize, PROCESS handleToProcess);
};
#include "errorLogging.h"
#include "imgui.h"
#include <memory.h>

void ErrorLog::renderText()
{
	if (errorLog[0] != 0)
	{
		if (errorType == info)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3, 0.3, 0.7, 1.0));
			ImGui::Text(errorLog);
			ImGui::PopStyleColor(1);
		}
		else if (errorType == warn)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7, 0.6, 0.3, 1.0));
			ImGui::Text("Warn: %s", errorLog);
			ImGui::PopStyleColor(1);
		}
		else if (errorType == error)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7, 0.3, 0.3, 1.0));
			ImGui::Text("Error: %s", errorLog);
			ImGui::PopStyleColor(1);
		}

	}
}

void ErrorLog::clearError()
{
	memset(errorLog, 0, sizeof(errorLog));
}

void ErrorLog::setError(const char* e, ErrorType type)
{
	strcpy(errorLog, e);
	errorType = type;
}

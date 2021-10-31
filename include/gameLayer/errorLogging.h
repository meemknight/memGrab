#pragma once

struct ErrorLog
{
	char errorLog[256] = {};
	
	enum ErrorType : int
	{
		info = 0,
		warn,
		error,
	};

	int errorType = ErrorType::error;

	void renderText();
	void clearError();
	void setError(const char* e, ErrorType type = ErrorType::error);
};
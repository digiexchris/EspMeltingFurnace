#pragma once

#include "esp_log.h"
#include <string>

class Console
{
public:
	Console();
	static Console *GetInstance()
	{
		if (myInstance == nullptr)
		{
			myInstance = new Console();
		}
		return myInstance;
	}

private:
	static void ConsoleTask(void *arg);
	void InitializeConsole();
	void InitializeConsoleLib();
	void SetupPrompt(const char *prompt_str);

	std::string prompt;
	static Console *myInstance;
};
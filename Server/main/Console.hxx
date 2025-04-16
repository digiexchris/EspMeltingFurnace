#pragma once

#include "esp_log.h"
#include <string>

#include "hardware.h"

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

	~Console()
	{
		if (myInstance)
		{
			delete myInstance;
			myInstance = nullptr;
		}
	}

private:
	static void ConsoleTask(void *arg);
	static void StatusTask(void *arg);
	void InitializeConsole();
	void InitializeConsoleLib();
	void SetupPrompt(const char *prompt_str);

	static int Temp(int argc, char **argv);
	static int GetPwmDutyCycle(int argc, char **argv);
	static int Heating(int argc, char **argv);

#if SIMULATED_TEMP_DEVICE
	static int SetSimThermocoupleTemp(int argc, char **argv);
#endif

	std::string prompt;
	static Console *myInstance;
};
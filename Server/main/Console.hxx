#pragma once

#include "esp_log.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
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
	void InitializeConsole();

	void CreateWindow();

	static void HandleKeypress(int key);
	static void IncrementTemp(int increment);
	static void SetHeating(bool enabled);

	static int Temp(int argc, char **argv);
	static int GetPwmDutyCycle(int argc, char **argv);
	static int Heating(int argc, char **argv);
	static int Status(int argc, char **argv);
	static void StatusOverlayTask(void *arg);

#if SIMULATED_TEMP_DEVICE
	static int SetSimThermocoupleTemp(int argc, char **argv);
#endif

	std::string prompt;
	static Console *myInstance;

	ftxui::Component LeftWindowContent()
	{
		class Impl : public ftxui::ComponentBase
		{
		private:
			bool checked[3] = {false, false, false};
			float slider = 50;

		public:
			Impl()
			{
				Add(ftxui::Container::Vertical({
					ftxui::Checkbox("Check me", &checked[0]),
					ftxui::Checkbox("Check me", &checked[1]),
					ftxui::Checkbox("Check me", &checked[2]),
					ftxui::Slider("Slider", &slider, 0.f, 100.f),
				}));
			}
		};
		return ftxui::Make<Impl>();
	}
};
#include "Console.hxx"

#include "argtable3/argtable3.h"
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_cdcacm.h"
#include "linenoise/linenoise.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmd_system.h"
#include "cmd_wifi.h"

#include "State.hxx"
#include "TempController.hxx"
#include "hardware.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

#define CONSOLE_MAX_CMDLINE_ARGS 8
#define CONSOLE_MAX_CMDLINE_LENGTH 256
#define CONSOLE_PROMPT_MAX_LEN (32)

Console *Console::myInstance = nullptr;

void Console::InitializeConsole()
{
	/* Drain stdout before reconfiguring it */
	fflush(stdout);
	fsync(fileno(stdout));

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
	/* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
	uart_vfs_dev_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
	/* Move the caret to the beginning of the next line on '\n' */
	uart_vfs_dev_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

	/* Configure UART. Note that REF_TICK is used so that the baud rate remains
	 * correct while APB frequency is changing in light sleep mode.
	 */
	const uart_config_t uart_config = {
		.baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
		.source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
		.source_clk = UART_SCLK_XTAL,
#endif
	};
	/* Install UART driver for interrupt-driven reads and writes */
	auto err = uart_driver_install((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0);

	if (err != ESP_OK && err != ESP_FAIL)
	{
		ESP_ERROR_CHECK(err);
	}
	ESP_ERROR_CHECK(uart_param_config((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

	/* Tell VFS to use UART driver */
	uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
	/* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
	esp_vfs_dev_cdcacm_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
	/* Move the caret to the beginning of the next line on '\n' */
	esp_vfs_dev_cdcacm_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

	/* Enable blocking mode on stdin and stdout */
	fcntl(fileno(stdout), F_SETFL, 0);
	fcntl(fileno(stdin), F_SETFL, 0);

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
	/* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
	usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
	/* Move the caret to the beginning of the next line on '\n' */
	usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

	/* Enable blocking mode on stdin and stdout */
	fcntl(fileno(stdout), F_SETFL, 0);
	fcntl(fileno(stdin), F_SETFL, 0);

	usb_serial_jtag_driver_config_t jtag_config = {
		.tx_buffer_size = 256,
		.rx_buffer_size = 256,
	};

	/* Install USB-SERIAL-JTAG driver for interrupt-driven reads and writes */
	ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&jtag_config));

	/* Tell vfs to use usb-serial-jtag driver */
	usb_serial_jtag_vfs_use_driver();

#else
#error Unsupported console type
#endif

	/* Disable buffering on stdin */
	setvbuf(stdin, NULL, _IONBF, 0);

	// Set terminal to raw mode for keypress detection
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
	term.c_cc[VMIN] = 0;			  // Return immediately even if no char available
	term.c_cc[VTIME] = 0;			  // No timeout
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

Console::Console()
{
	/* Initialize console output periheral (UART, USB_OTG, USB_JTAG) */
	InitializeConsole();

	CreateWindow();

	/* Register commands */
	// esp_console_register_help_command();
	// register_system_common();

#if SOC_WIFI_SUPPORTED
	// register_wifi();
#endif

	const esp_console_cmd_t cmd3 = {
		.command = "temp",
		.help = "Get/Set the target temperature and get current temp\n"
				"Usage: temp <t<temp> | r<rate> | i<time>>\n"
				"t<temp> - Set target temperature\n"
				"r<rate> - Set heating rate\n"
				"i<rate> - Set heating rate by how much time in minutes it should take to get there from the current temp. Mutually exclusive with r",
		.hint = NULL,
		.func = &Temp,
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd3));

	const esp_console_cmd_t cmd4 = {
		.command = "getPwmDutyCycle",
		.help = "Get the current PWM duty cycle",
		.hint = NULL,
		.func = &GetPwmDutyCycle,
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd4));

	const esp_console_cmd_t cmd5 = {
		.command = "heating",
		.help = "Set/Get the heating state (on/off)",
		.hint = NULL,
		.func = &Heating,
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd5));

	const esp_console_cmd_t cmd6 = {
		.command = "status",
		.help = "Get the current status of the system",
		.hint = NULL,
		.func = &Status,
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd6));

	const esp_console_cmd_t defaultCmd = {
		.command = "s",
		.help = "Get the current status of the system",
		.hint = NULL,
		.func = &Status,
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&defaultCmd));

#if SIMULATED_TEMP_DEVICE
	const esp_console_cmd_t cmd2 = {
		.command = "setSimTemp",
		.help = "Set the simulated thermocouple temp output",
		.hint = NULL,
		.func = &SetSimThermocoupleTemp,
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd2));
#endif

	printf("\033[2J\033[H");

	printf("\n"
		   "Type 'help' to get the list of commands.\n"
		   "Use UP/DOWN arrows to navigate through command history.\n"
		   "Press TAB when typing command name to auto-complete.\n");

	/* Figure out if the terminal supports escape sequences */
	int probe_status = linenoiseProbe();
	if (probe_status)
	{ /* zero indicates success */
		printf("\n"
			   "Your terminal application does not support escape sequences.\n"
			   "Line editing and history features are disabled.\n"
			   "On Windows, try using Putty instead.\n");
		linenoiseSetDumbMode(1);
	}

	xTaskCreate(ConsoleTask, "ConsoleTask", 4096, this, 10, NULL);
	xTaskCreate(StatusOverlayTask, "StatusOverlayTask", 4096, this, 10, NULL);
}

void Console::CreateWindow()
{
	using namespace ftxui;

	// Initialize the screen
	auto screen = ScreenInteractive::TerminalOutput();

	// Create your UI components
	auto component = Container::Vertical({
		Button("Increment Temperature", [&]
			   { IncrementTemp(25); }),
		Button("Decrement Temperature", [&]
			   { IncrementTemp(-25); }),
		Button("Toggle Heating", [&]
			   { SetHeating(!State::GetInstance()->IsEnabled()); }),
	});

	// Run the UI
	screen.Loop(component);
}

int Console::Temp(int argc, char **argv)
{
	TempController *controller = TempController::GetInstance();
	if (argc >= 2)
	{
		double temp = 0;
		double rate = 0;
		double time = 0;
		bool setTemp = false;
		bool setRate = false;
		bool setTime = false;

		for (int i = 1; i < argc; i++)
		{
			if (argv[i][0] == 't' || argv[i][0] == 'T')
			{
				if (sscanf(argv[i] + 1, "%lf", &temp) == 1)
				{
					setTemp = true;
				}
				else
				{
					printf("Invalid temperature format\n");
					return 1;
				}
			}

			if (argv[i][0] == 'r' || argv[i][0] == 'R')
			{
				if (sscanf(argv[i] + 1, "%lf", &rate) == 1)
				{
					setRate = true;
				}
				else
				{
					printf("Invalid rate format\n");
					return 1;
				}
			}

			if (argv[i][0] == 'i' || argv[i][0] == 'I')
			{
				if (sscanf(argv[i] + 1, "%lf", &time) == 1)
				{
					setTime = true;
				}
				else
				{
					printf("Invalid rate format\n");
					return 1;
				}
			}
		}

		if (setRate && setTime)
		{
			printf("r and i rates are mutually exclusive!\n");
			return 1;
		}

		if (setTemp)
		{
			if (temp < MIN_TEMP)
			{
				temp = MIN_TEMP;
			}

			if (temp > MAX_TEMP)
			{
				temp = MAX_TEMP;
			}

			controller->SetTargetTemp(temp);
			printf("Set target temperature: %.2f\n", temp);
		}

		if (setRate)
		{
			if (rate > MAX_HEATING_RATE_PER_SECOND * 60.0f)
			{
				rate = MAX_HEATING_RATE_PER_SECOND * 60.0f;
			}
			controller->SetHeatingRate(rate);
			printf("Set heating rate: %.2f\n", rate);
		}

		if (setTime)
		{
			if (time <= 0)
			{
				printf("Time out of range: %.2f\n", time);
				return 1;
			}

			int ratePerMinute = int((controller->GetTargetTemp() - controller->GetCurrentTemp()) / time);

			controller->SetHeatingRate(ratePerMinute / 60.0f);

			printf("Set heating rate: %.2f/s\n", ratePerMinute / 60.0f);
		}
	}
	else
	{
		printf("Usage: setTemp <temp>\n");
		printf("Current temperature: %.2f\n", controller->GetCurrentTemp());
		printf("Set temperature: %.2f\n", controller->GetTargetTemp());
	}
	return 0;
}

int Console::GetPwmDutyCycle(int argc, char **argv)
{
	TempController *controller = TempController::GetInstance();
	if (argc == 1)
	{
		printf("Current PWM duty cycle: %d\n", controller->GetPwmDutyCycle());
	}
	else
	{
		printf("Usage: getPwmDutyCycle\n");
	}
	return 0;
}

int Console::Status(int argc, char **argv)
{
	TempController *controller = TempController::GetInstance();
	State *state = State::GetInstance();

	printf("Current temperature: %.2f\n", controller->GetCurrentTemp());
	printf("Set temperature: %.2f\n", controller->GetTargetTemp());
	printf("Heating %s\n", state->IsEnabled() ? "enabled" : "disabled");
	printf("PWM duty cycle: %d\n", controller->GetPwmDutyCycle());
	printf("Heating rate: %.2f\n", controller->GetConfig().HEATING_RATE_PER_SECOND);
	printf("Internal Target Temp: %.2f\n", controller->GetInternalSetTemp());
	return 0;
}

int Console::Heating(int argc, char **argv)
{
	if (argc == 2)
	{
		bool enabled = false;

		// Check for "on" string
		if (strcmp(argv[1], "on") == 0 || strcmp(argv[1], "1") == 0)
		{
			enabled = true;
		}
		else if (strcmp(argv[1], "off") == 0 || strcmp(argv[1], "0") == 0)
		{
			enabled = false;
		}
		else
		{
			printf("Invalid argument. Use 'on'/'1' or 'off'/'0'\n");
			return 1;
		}

		State *state = State::GetInstance();
		state->SetEnabled(enabled);
		printf("Heating %s\n", enabled ? "enabled" : "disabled");
	}
	else
	{
		printf("Usage: setHeating <0|1>\n");
		printf("Heating %s\n", State::GetInstance()->IsEnabled() ? "enabled" : "disabled");
	}
	return 0;
}

#if SIMULATED_TEMP_DEVICE
int Console::SetSimThermocoupleTemp(int argc, char **argv)
{

	if (argc == 2)
	{
		TempController *controller = TempController::GetInstance();
		double temp = atof(argv[1]);
		SimulatedTempDevice *simulatedThermocouple = static_cast<SimulatedTempDevice *>(controller->GetTempDevice());
		simulatedThermocouple->SetTemp(temp);
		printf("Set temperature: %.2f\n", temp);
	}
	else
	{
		printf("Usage: setSimTemp <temp>\n");
	}
	return 0;
}
#endif

void Console::ConsoleTask(void *arg)
{
	Console *console = static_cast<Console *>(arg);
	/* Main loop */
	while (42)
	{
		// Read a single character without echo
		char c;
		if (read(STDIN_FILENO, &c, 1) > 0)
		{
			// Check for escape sequences (arrow keys, etc.)
			if (c == 27)
			{ // ESC character
				char seq[3];

				// Read up to 2 more chars to identify the sequence
				if (read(STDIN_FILENO, &seq[0], 1) > 0)
				{
					if (seq[0] == '[')
					{
						if (read(STDIN_FILENO, &seq[1], 1) > 0)
						{
							// Handle arrow keys
							switch (seq[1])
							{
							case 'A': // Up arrow
								IncrementTemp(25);
								break;
							case 'B': // Down arrow
								IncrementTemp(-25);
								break;
							}
						}
					}
				}
			}
			// Check for ctrl+shift+s (19 is the ASCII value for ctrl+s)
			else if (c == 19)
			{
				// ToggleHeating(true);
			}
			// Check for spacebar
			else if (c == ' ')
			{
				// ToggleHeating(false);
			}

			// Display status after any keypress
			Status(0, NULL);
		}

		// Small delay to prevent CPU hogging
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

void Console::IncrementTemp(int increment)
{
	TempController *controller = TempController::GetInstance();
	float currentTarget = controller->GetTargetTemp();
	float newTarget = currentTarget + increment;

	// Apply limits
	if (newTarget < MIN_TEMP)
	{
		newTarget = MIN_TEMP;
	}
	if (newTarget > MAX_TEMP)
	{
		newTarget = MAX_TEMP;
	}

	controller->SetTargetTemp(newTarget);
	printf("\nTemperature %s to %.1f°C\n",
		   (increment > 0) ? "increased" : "decreased",
		   newTarget);
}

void Console::SetHeating(bool enabled)
{
	State *state = State::GetInstance();
	state->SetEnabled(enabled);
	printf("\nHeating %s\n", enabled ? "enabled" : "disabled");
}

void Console::StatusOverlayTask(void *arg)
{

	while (42)
	{
		printf("\n\n");
		printf("Keyboard Controls:\n");
		printf("UP ARROW    - Increase temperature by 25°C\n");
		printf("DOWN ARROW  - Decrease temperature by 25°C\n");
		printf("CTRL+SHIFT+S - Start heating\n");
		printf("SPACE       - Stop heating\n");
		printf("\n");

		// Save cursor position
		printf("\033[s");

		// // Move to top right corner (1st row, 20 characters from right edge)
		// printf("\033[1;%dH", 80 - 30);

		// // Clear the area (8 rows, 20 columns)
		// for (int i = 0; i < 8; i++)
		// {
		// 	printf("\033[%d;%dH", 1 + i, 80 - 30);
		// 	printf("%-20s", ""); // Clear line with spaces
		// }

		// Move back to top right and print status
		printf("\033[1;%dH    ", 80 - 30);

		TempController *controller = TempController::GetInstance();
		State *state = State::GetInstance();

		printf("Temp: %.1f°C    ", controller->GetCurrentTemp());
		printf("\033[2;%dH", 80 - 30);
		printf("Target: %.1f°C     ", controller->GetTargetTemp());
		printf("\033[3;%dH", 80 - 30);
		printf("Heating: %s     ", state->IsEnabled() ? "ON " : "OFF");
		printf("\033[4;%dH", 80 - 30);
		printf("PWM: %d%%         ", controller->GetPwmDutyCycle());
		printf("\033[5;%dH", 80 - 30);
		printf("Rate: %.2f°C/s        ", controller->GetConfig().HEATING_RATE_PER_SECOND);
		printf("\033[6;%dH", 80 - 30);
		printf("Internal Setpoint : %.2f°C      ", controller->GetInternalSetTemp());
		// Restore cursor position
		printf("\033[u");

		// Update every second
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}
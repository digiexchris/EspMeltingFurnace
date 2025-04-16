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
}

void Console::InitializeConsoleLib()
{
	/* Initialize the console */
	esp_console_config_t console_config = {
		.max_cmdline_length = CONSOLE_MAX_CMDLINE_LENGTH,
		.max_cmdline_args = CONSOLE_MAX_CMDLINE_ARGS,

#if CONFIG_LOG_COLORS
		.hint_color = atoi(LOG_COLOR_CYAN)
#endif
	};
	ESP_ERROR_CHECK(esp_console_init(&console_config));

	/* Configure linenoise line completion library */
	/* Enable multiline editing. If not set, long commands will scroll within
	 * single line.
	 */
	linenoiseSetMultiLine(1);

	/* Tell linenoise where to get command completions and hints */
	linenoiseSetCompletionCallback(&esp_console_get_completion);
	linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);

	/* Set command history size */
	linenoiseHistorySetMaxLen(100);

	/* Set command maximum length */
	linenoiseSetMaxLineLen(console_config.max_cmdline_length);

	/* Don't return empty lines */
	linenoiseAllowEmpty(false);

#if CONFIG_CONSOLE_STORE_HISTORY
	/* Load command history from filesystem */
	linenoiseHistoryLoad(history_path);
#endif // CONFIG_CONSOLE_STORE_HISTORY

	/* Figure out if the terminal supports escape sequences */
	const int probe_status = linenoiseProbe();
	if (probe_status)
	{ /* zero indicates success */
		linenoiseSetDumbMode(1);
	}
}

void Console::SetupPrompt(const char *prompt_str)
{
	/* set command line prompt */
	const char *prompt_temp = "esp>";
	if (prompt_str)
	{
		prompt_temp = prompt_str;
	}
	prompt = std::string(LOG_COLOR_I) + prompt_temp + " " + std::string(LOG_RESET_COLOR);

	if (linenoiseIsDumbMode())
	{
#if CONFIG_LOG_COLORS
		/* Since the terminal doesn't support escape sequences,
		 * don't use color codes in the s_prompt.
		 */
		snprintf(prompt, CONSOLE_PROMPT_MAX_LEN - 1, "%s ", prompt_temp);
#endif // CONFIG_LOG_COLORS
	}
}

Console::Console()
{
	/* Initialize console output periheral (UART, USB_OTG, USB_JTAG) */
	InitializeConsole();

	/* Initialize linenoise library and esp_console*/
	InitializeConsoleLib();

	SetupPrompt("console>");

	/* Register commands */
	esp_console_register_help_command();
	register_system_common();

#if SOC_WIFI_SUPPORTED
	register_wifi();
#endif

	const esp_console_cmd_t cmd3 = {
		.command = "temp",
		.help = "Get/Set the target temperature and get current temp",
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

#if SIMULATED_TEMP_DEVICE
	const esp_console_cmd_t cmd2 = {
		.command = "setSimTemp",
		.help = "Set the simulated thermocouple temp output",
		.hint = NULL,
		.func = &SetSimThermocoupleTemp,
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd2));
#endif

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
	xTaskCreate(StatusTask, "StatusTask", 4096, this, 10, NULL);
}

int Console::Temp(int argc, char **argv)
{
	TempController *controller = TempController::GetInstance();
	if (argc == 2)
	{
		double temp = atof(argv[1]);
		controller->SetTemp(temp);
		printf("Set temperature: %.2f\n", temp);
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

int Console::Heating(int argc, char **argv)
{
	TempController *controller = TempController::GetInstance();
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
		/* Get a line using linenoise.
		 * The line is returned when ENTER is pressed.
		 */
		char *line = linenoise(console->prompt.c_str());
#if CONFIG_CONSOLE_IGNORE_EMPTY_LINES
		if (line == NULL)
		{ /* Ignore empty lines */
			continue;
			;
		}
#else
		if (line == NULL)
		{ /* Try again on EOF or error */
			continue;
		}
#endif // CONFIG_CONSOLE_IGNORE_EMPTY_LINES

		/* Add the command to the history if not empty*/
		if (strlen(line) > 0)
		{
			linenoiseHistoryAdd(line);
#if CONFIG_CONSOLE_STORE_HISTORY
			/* Save command history to filesystem */
			linenoiseHistorySave(HISTORY_PATH);
#endif // CONFIG_CONSOLE_STORE_HISTORY
		}

		/* Try to run the command */
		int ret;
		esp_err_t err = esp_console_run(line, &ret);
		if (err == ESP_ERR_NOT_FOUND)
		{
			printf("Unrecognized command\n");
		}
		else if (err == ESP_ERR_INVALID_ARG)
		{
			// command was empty
		}
		else if (err == ESP_OK && ret != ESP_OK)
		{
			printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
		}
		else if (err != ESP_OK)
		{
			printf("Internal error: %s\n", esp_err_to_name(err));
		}
		/* linenoise allocates line buffer on the heap, so need to free it */
		linenoiseFree(line);
	}
}

void Console::StatusTask(void *arg)
{
	TempController *controller = TempController::GetInstance();
	State *state = State::GetInstance();

	while (42)
	{
		printf("Current temperature: %.2f\n", controller->GetCurrentTemp());
		printf("Set temperature: %.2f\n", controller->GetTargetTemp());
		printf("Heating %s\n", state->IsEnabled() ? "enabled" : "disabled");
		printf("PWM duty cycle: %d\n", controller->GetPwmDutyCycle());

		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}
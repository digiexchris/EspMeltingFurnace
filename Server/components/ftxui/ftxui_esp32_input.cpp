#ifdef ESP_PLATFORM
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "FTXUI_Input";

// Forward declarations for functions defined in ftxui_esp32_signal.cpp
extern "C"
{
	void esp_ftxui_send_sigint();
	void esp_ftxui_send_sigterm();
	void esp_ftxui_send_sigwinch();
	void esp_post_ftxui_signal(int sig);
}

// Task to handle keyboard input and translate to FTXUI events
static void ftxui_input_task(void *arg)
{
	ESP_LOGI(TAG, "Starting FTXUI input handler task");

	const int buf_size = 128;
	uint8_t data[buf_size];

	while (true)
	{
		// Read data from UART
		int len = uart_read_bytes(static_cast<uart_port_t>(CONFIG_ESP_CONSOLE_UART_NUM), data, buf_size,
								  pdMS_TO_TICKS(100));

		if (len > 0)
		{
			// Process special key sequences
			for (int i = 0; i < len; i++)
			{
				// Check for Ctrl+C (ASCII 3)
				if (data[i] == 3)
				{
					ESP_LOGD(TAG, "Detected Ctrl+C, sending SIGINT");
					esp_ftxui_send_sigint();
				}

				// Check for ESC sequence for terminal resize
				// This is a simplified example - actual resize detection
				// would need to parse VT100 sequences from terminal
				if (i + 2 < len && data[i] == 27 && data[i + 1] == '[' &&
					data[i + 2] == 'r')
				{
					ESP_LOGD(TAG, "Detected terminal resize sequence, sending SIGWINCH");
					esp_ftxui_send_sigwinch();
					i += 2; // Skip the parsed sequence
				}

				// Forward character to FTXUI input handling
				// ...
			}
		}

		// Small delay to prevent CPU hogging
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

// Initialize the input task
static void init_ftxui_input()
{
	static bool initialized = false;
	if (!initialized)
	{
		xTaskCreate(ftxui_input_task, "ftxui_input", 4096, NULL, 5, NULL);
		initialized = true;
	}
}

// Automatic initialization
static class FTXUIInputInit
{
public:
	FTXUIInputInit()
	{
		init_ftxui_input();
	}
} ftxui_input_init;

#endif // ESP_PLATFORM
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <functional>
#include <map>

#ifdef ESP_PLATFORM

static const char *TAG = "FTXUI_Signal";

// Define ESP event base for FTXUI events
ESP_EVENT_DECLARE_BASE(FTXUI_EVENT);
ESP_EVENT_DEFINE_BASE(FTXUI_EVENT);

// Signal event types
enum
{
	FTXUI_SIGINT_EVENT = 2,	  // Corresponds to SIGINT
	FTXUI_SIGTERM_EVENT = 15, // Corresponds to SIGTERM
	FTXUI_SIGWINCH_EVENT = 28 // Corresponds to SIGWINCH
};

// Map of signal handlers for ESP32
static std::map<int, std::function<void(int)>> signal_handlers;

// Mock implementation of signal functionality for ESP32
extern "C"
{
	typedef void (*sighandler_t)(int);

	sighandler_t signal(int sig, sighandler_t handler)
	{
		ESP_LOGD(TAG, "Setting signal handler for signal %d", sig);

		// Store the old handler to return
		sighandler_t old_handler = nullptr;
		auto it = signal_handlers.find(sig);
		if (it != signal_handlers.end())
		{
			// Convert std::function back to function pointer (this is a simplification)
			old_handler = reinterpret_cast<sighandler_t>(it->second.target<void (*)(int)>());
		}

		// Store new handler
		signal_handlers[sig] = std::function<void(int)>(handler);

		return old_handler;
	}

// These definitions might be needed if not provided by ESP-IDF
#ifndef SIGINT
#define SIGINT 2
#endif

#ifndef SIGTERM
#define SIGTERM 15
#endif

#ifndef SIGWINCH
#define SIGWINCH 28
#endif
}

// ESP event handler
static void ftxui_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
	int sig = id;
	ESP_LOGD(TAG, "Received FTXUI event for signal %d", sig);

	auto it = signal_handlers.find(sig);
	if (it != signal_handlers.end() && it->second)
	{
		it->second(sig);
	}
}

// Initialize ESP event loop for FTXUI
static void init_esp_event_system()
{
	static bool initialized = false;
	if (!initialized)
	{
		ESP_LOGI(TAG, "Initializing FTXUI event system");
		ESP_ERROR_CHECK(esp_event_loop_create_default());
		ESP_ERROR_CHECK(esp_event_handler_register(FTXUI_EVENT, ESP_EVENT_ANY_ID,
												   ftxui_event_handler, NULL));
		initialized = true;
	}
}

// Functions to send signals within the ESP32 environment
extern "C"
{
	// Post an event to simulate a signal
	void esp_post_ftxui_signal(int sig)
	{
		ESP_LOGD(TAG, "Posting FTXUI signal event %d", sig);
		init_esp_event_system();
		int signal_data = sig;
		esp_event_post(FTXUI_EVENT, sig, &signal_data, sizeof(signal_data), portMAX_DELAY);
	}

	// Helper functions for common signals
	void esp_ftxui_send_sigint()
	{
		esp_post_ftxui_signal(SIGINT);
	}

	void esp_ftxui_send_sigterm()
	{
		esp_post_ftxui_signal(SIGTERM);
	}

	void esp_ftxui_send_sigwinch()
	{
		esp_post_ftxui_signal(SIGWINCH);
	}
}

// Automatic initialization
static class FTXUISignalInit
{
public:
	FTXUISignalInit()
	{
		init_esp_event_system();
	}
} ftxui_signal_init;

#endif // ESP_PLATFORM
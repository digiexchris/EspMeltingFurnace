#include "GPIO.hxx"
#include "hardware.h"
#include <esp_log.h>

#include "hardware.h"

static const char *GPIOTAG = "GPIO";

QueueHandle_t GPIOManager::myEventQueue = nullptr;
GPIOManager *GPIOManager::myInstance = nullptr;

enum class EventType
{
	DOOR_SWITCH_PIN,
};

// Event structure for queue
struct InterruptEvent
{
	EventType type;
};

GPIOManager::GPIOManager()
{
	myInstance = this;
	ESP_LOGI(GPIOTAG, "Initializing GPIO Manager");

	// Create event queue for ISR communication
	myEventQueue = xQueueCreate(10, sizeof(InterruptEvent));
	if (myEventQueue == nullptr)
	{
		ESP_LOGE(GPIOTAG, "Failed to create event queue");
		return;
	}

	// // Configure ESP GPIO pins for interrupts
	// gpio_config_t io_conf = {};
	// io_conf.intr_type = GPIO_INTR_NEGEDGE; // Falling edge trigger
	// io_conf.pin_bit_mask = (1ULL << MCP23017_I2C_INT0) | (1ULL << MCP23017_I2C_INT1);
	// io_conf.mode = GPIO_MODE_INPUT;
	// io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	// gpio_config(&io_conf);

	// Install GPIO ISR service
	gpio_install_isr_service(0);

	// Create the interrupt handler task
	xTaskCreate(interruptHandlerTask, "gpio_handler", 4096, this, 10, &myTaskHandle);
}

void GPIOManager::setEmergencyRelay(bool value)
{
}

void GPIOManager::interruptHandlerTask(void *arg)
{
	GPIOManager *gpio = static_cast<GPIOManager *>(arg);
	InterruptEvent evt;

	while (true)
	{
		// Wait for interrupt event
		if (xQueueReceive(myEventQueue, &evt, portMAX_DELAY))
		{

			// Handle encoder and switch based on port values
			// todo

			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
	}
}

void GPIOManager::processDoorSwitch()
{
	// TODO switch to traditional gpio

	if (myIsDoorOpen)
	{
		ESP_LOGI(GPIOTAG, "Door is open");
	}
	else
	{
		ESP_LOGI(GPIOTAG, "Door is closed");
	}
}
#include "GPIO.hxx"
#include "hardware.h"
#include <esp_log.h>

#include "hardware.h"

#include "uiTemp.hxx"

static const char *GPIOTAG = "GPIO";

QueueHandle_t GPIOManager::myEventQueue = nullptr;
GPIOManager *GPIOManager::myInstance = nullptr;

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
			//todo


			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
	}
}

void GPIOManager::processDoorSwitch()
{
	if (!myMCPInitialized || myMCP23017 == nullptr)
	{
		return;
	}

	uint8_t port_val = readMcpPortA();
	myIsDoorOpen = ((port_val >> MCP23017_DOOR_SW) & 0x01) == 0; // Active low

	if (myIsDoorOpen)
	{
		ESP_LOGI(GPIOTAG, "Door is open");
	}
	else
	{
		ESP_LOGI(GPIOTAG, "Door is closed");
	}
}

void GPIOManager::processToggleSwitch()
{
	if (!myMCPInitialized || myMCP23017 == nullptr)
	{
		return;
	}

	uint8_t port_val = readMcpPortA();
	bool switch_on = ((port_val >> MCP23017_SW1) & 0x01) == 0; // Active low

	// Only trigger event on state change
	if (switch_on != myLastSwitchState)
	{
		myLastSwitchState = switch_on;

		ESP_LOGI(GPIOTAG, "Toggle switch changed to: %s", switch_on ? "ON" : "OFF");

		// Send the appropriate event
		if (switch_on)
		{
			TempUI *tempUI = TempUI::GetInstance();
			tempUI->Start();
		}
		else
		{
			TempUI *tempUI = TempUI::GetInstance();
			tempUI->Stop();
		}
	}
}
// #include "sdkconfig.h"
// #include <driver/i2c.h>
// #include <esp_log.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <stdio.h>
void GPIOManager::scanI2CBus()
{
	ESP_LOGI(GPIOTAG, "Scanning I2C bus for devices...");

	// i2c_config_t conf;
	// conf.mode = I2C_MODE_MASTER;
	// conf.sda_io_num = I2C_SDA;
	// conf.scl_io_num = I2C_SCL;
	// conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	// conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	// conf.master.clk_speed = 100000;
	// i2c_param_config(I2C_NUM_0, &conf);

	// i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

	// int i;
	// esp_err_t espRc;
	// printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	// printf("00:         ");
	// for (i=3; i< 0x78; i++) {
	// 	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	// 	i2c_master_start(cmd);
	// 	i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	// 	i2c_master_stop(cmd);

	// 	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	// 	if (i%16 == 0) {
	// 		printf("\n%.2x:", i);
	// 	}
	// 	if (espRc == 0) {
	// 		printf(" %.2x", i);
	// 	} else {
	// 		printf(" --");
	// 	}
	// 	//ESP_LOGD(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
	// 	i2c_cmd_link_delete(cmd);
	// }
	// printf("\n");

	// ESP_LOGI(GPIOTAG, "Expected MCP23017 address: 0x%02X", MCP23017_I2C_ADDR);
}
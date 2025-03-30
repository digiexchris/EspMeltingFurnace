#include "GPIO.hxx"
#include "hardware.h"
#include <esp_log.h>
#include <esp_lvgl_port.h>
#include <i2c_bus.h>
#include <lvgl.h>
#include <mcp23017.h>

#include "hardware.h"

#include "uiTemp.hxx"

static const char *GPIOTAG = "GPIO";

// Define the static queue handle
QueueHandle_t GPIOManager::myEventQueue = nullptr;

// Event types for the queue
enum EventType
{
	EVENT_INT0, // Interrupt 0 triggered
	EVENT_INT1	// Interrupt 1 triggered
};

// Event structure for queue
struct InterruptEvent
{
	EventType type;
};

GPIOManager::GPIOManager()
{
	ESP_LOGI(GPIOTAG, "Initializing GPIO Manager");
	myI2CConfig = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = I2C_SDA,
		.scl_io_num = I2C_SCL,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master = {
			.clk_speed = I2C_MASTER_FREQ_HZ,
		}};

	// Create event queue for ISR communication
	myEventQueue = xQueueCreate(10, sizeof(InterruptEvent));
	if (myEventQueue == nullptr)
	{
		ESP_LOGE(GPIOTAG, "Failed to create event queue");
		return;
	}

	// Initialize I2C bus
	myI2CBus = i2c_bus_create(I2C_MASTER_NUM, &myI2CConfig);
	if (myI2CBus == nullptr)
	{
		ESP_LOGE(GPIOTAG, "Failed to create I2C bus");
		return;
	}

	// Initialize MCP23017
	myMCP23017 = mcp23017_create(myI2CBus, MCP23017_I2C_ADDR);
	if (myMCP23017 == nullptr)
	{
		ESP_LOGE(GPIOTAG, "Failed to create MCP23017, scanning I2C bus for available devices");
		scanI2CBus();
		ESP_LOGW(GPIOTAG, "Continuing without MCP23017 functionality");
		myMCPInitialized = false;
	}
	else
	{
		// Configure the MCP23017
		configureMcp();
		myMCPInitialized = true;

		// Default encoder increment to 1
		myEncoderIncrement = 1;

		// Configure ESP GPIO pins for interrupts
		gpio_config_t io_conf = {};
		io_conf.intr_type = GPIO_INTR_NEGEDGE; // Falling edge trigger
		io_conf.pin_bit_mask = (1ULL << MCP23017_I2C_INT0) | (1ULL << MCP23017_I2C_INT1);
		io_conf.mode = GPIO_MODE_INPUT;
		io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
		gpio_config(&io_conf);

		// Install GPIO ISR service
		gpio_install_isr_service(0);

		// Add ISR handlers for both interrupt pins
		gpio_isr_handler_add(MCP23017_I2C_INT0, gpioInt0IsrHandler, this);
		gpio_isr_handler_add(MCP23017_I2C_INT1, gpioInt1IsrHandler, this);

		// Create the interrupt handler task
		xTaskCreate(interruptHandlerTask, "gpio_handler", 4096, this, 10, &myTaskHandle);
	}
}

void GPIOManager::setArc(lv_obj_t *arc)
{
	myArc = arc;
}

void GPIOManager::setButton(lv_obj_t *button)
{
	myButton = button;
}

void GPIOManager::configureMcp()
{
	// Define the pin mask for used pins only (pins 0-3)
	// 0: MCP23017_SW1 - Toggle switch
	// 1: MCP23017_ENC_SW - Encoder push button
	// 2: MCP23017_ENC_A - Encoder pin A
	// 3: MCP23017_ENC_B - Encoder pin B
	uint8_t usedPinsMask = (1 << MCP23017_SW1) |
						   (1 << MCP23017_ENC_SW) |
						   (1 << MCP23017_ENC_A) |
						   (1 << MCP23017_ENC_B);

	// Configure only used pins as inputs (1 = input)
	mcp23017_set_io_dir(myMCP23017, usedPinsMask, MCP23017_GPIOA);

	// Enable pull-ups only for used pins
	mcp23017_set_pullup(myMCP23017, usedPinsMask);

	// Enable interrupts only for used pins - convert to uint16_t format for port A
	uint16_t interruptPinsMask = (uint16_t)usedPinsMask;
	mcp23017_interrupt_en(myMCP23017, interruptPinsMask, false, 0);

	// Read initial state
	myLastEncoderState = readMcpPortA();
	myLastSwitchState = (myLastEncoderState & (1 << MCP23017_SW1)) == 0; // Active low

	// Clear any pending interrupts by reading the capture register
	mcp23017_get_int_flag(myMCP23017);
}

uint8_t GPIOManager::readMcpPortA()
{
	// Use the correct MCP23017 API function to read the I/O port A
	return mcp23017_read_io(myMCP23017, MCP23017_GPIOA);
}

void IRAM_ATTR GPIOManager::gpioInt0IsrHandler(void *arg)
{
	// Queue event from ISR
	InterruptEvent evt = {EVENT_INT0};
	xQueueSendFromISR(myEventQueue, &evt, NULL);
}

void IRAM_ATTR GPIOManager::gpioInt1IsrHandler(void *arg)
{
	// Queue event from ISR
	InterruptEvent evt = {EVENT_INT1};
	xQueueSendFromISR(myEventQueue, &evt, NULL);
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
			// Read the MCP23017 port to get current states
			uint8_t port_val = gpio->readMcpPortA();

			// Handle encoder and switch based on port values
			gpio->processEncoder();
			gpio->processToggleSwitch();
			gpio->processEncoderButton();

			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
	}
}

void GPIOManager::processEncoder()
{
	if (!myMCPInitialized || myMCP23017 == nullptr)
	{
		return;
	}

	uint8_t port_val = readMcpPortA();
	uint8_t enc_state = ((port_val >> MCP23017_ENC_A) & 0x01) | (((port_val >> MCP23017_ENC_B) & 0x01) << 1);
	uint8_t prev_state = ((myLastEncoderState >> MCP23017_ENC_A) & 0x01) |
						 (((myLastEncoderState >> MCP23017_ENC_B) & 0x01) << 1);

	// Update last state
	myLastEncoderState = port_val;

	// Standard gray code decoding for rotary encoder
	if (enc_state != prev_state)
	{
		TempUI *tempUI = TempUI::GetInstance();
		// Determine direction based on gray code sequence
		switch (prev_state)
		{
		case 0:
			if (enc_state == 1)
			{
				int value = tempUI->GetTargetTemp();
				// Counter-clockwise - decrease value
				value -= myEncoderIncrement;
				if (value < 10)
				{
					value = 10;
				}
				tempUI->SetTargetTemp(value);
			}
			break;
		case 1:
			if (enc_state == 0)
			{
				int value = tempUI->GetTargetTemp();
				// Clockwise - increase value
				value += myEncoderIncrement;
				if (value > 1350)
				{
					value = 1350;
				}
				tempUI->SetTargetTemp(value);
			}
			break;
		case 2:
			if (enc_state == 3)
			{
				int value = tempUI->GetTargetTemp();
				// Clockwise - increase value
				value += myEncoderIncrement;
				if (value > 1350)
				{
					value = 1350;
				}
				tempUI->SetTargetTemp(value);
			}
			break;
		case 3:
			if (enc_state == 2)
			{
				int value = tempUI->GetTargetTemp();
				// Counter-clockwise - decrease value
				value -= myEncoderIncrement;
				if (value < 10)
				{
					value = 10;
				}
				tempUI->SetTargetTemp(value);
			}
			break;
		default:
			ESP_LOGE(GPIOTAG, "Invalid encoder state: %d", enc_state);
			assert(0); // Should never reach here

			break;
		}
	}
}

void GPIOManager::processEncoderButton()
{
	if (!myMCPInitialized || myMCP23017 == nullptr)
	{
		return;
	}

	uint8_t port_val = readMcpPortA();
	bool button_pressed = ((port_val >> MCP23017_ENC_SW) & 0x01) == 0; // Active low

	// Detect button press (transition from high to low)
	static bool last_button_state = false;

	if (button_pressed && !last_button_state)
	{
		ESP_LOGI(GPIOTAG, "Encoder button pressed");

		// Cycle through increment values: 1 -> 10 -> 100 -> 1
		if (myEncoderIncrement == 1)
		{
			myEncoderIncrement = 10;
		}
		else if (myEncoderIncrement == 10)
		{
			myEncoderIncrement = 100;
		}
		else
		{
			myEncoderIncrement = 1;
		}

		ESP_LOGI(GPIOTAG, "Encoder increment set to %d", myEncoderIncrement);
	}

	last_button_state = button_pressed;
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
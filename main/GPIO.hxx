#pragma once

#include "hardware.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <i2c_bus.h>
#include <lvgl.h>
#include <mcp23017.h>

class GPIOManager
{
public:
	GPIOManager();

	static GPIOManager *GetInstance()
	{
		return myInstance;
	}

	void setEmergencyRelay(bool value);
	bool isDoorOpen()
	{
		return myIsDoorOpen;
	};

private:
	// Set LVGL objects to control
	void setArc(lv_obj_t *arc);
	void setButton(lv_obj_t *button);
	static GPIOManager *myInstance;
	i2c_config_t myI2CConfig;

	i2c_bus_handle_t myI2CBus = nullptr;
	mcp23017_handle_t myMCP23017 = nullptr;
	bool myMCPInitialized = false;

	// For encoder increment toggle
	int myEncoderIncrement = 1;

	// Last encoder state for quadrature decoding
	uint8_t myLastEncoderState = 0;

	// Last switch state to detect changes
	bool myLastSwitchState = false;
	bool myIsDoorOpen = true;

	// LVGL objects to control
	lv_obj_t *myArc = nullptr;
	lv_obj_t *myButton = nullptr;

	// Interrupt handler task
	static void interruptHandlerTask(void *arg);
	TaskHandle_t myTaskHandle = nullptr;

	// GPIO interrupt handlers
	static void IRAM_ATTR gpioInt0IsrHandler(void *arg);
	static void IRAM_ATTR gpioInt1IsrHandler(void *arg);

	// Process encoder rotation
	void processEncoder();

	// Process encoder button press
	void processEncoderButton();

	// Process toggle switch
	void processToggleSwitch();

	void processDoorSwitch();

	// Read MCP23017 state
	uint8_t readMcpPortA();

	uint8_t readMcpPortB();

	void writeMcpPortB(uint8_t value);

	void writeMcpPortA(uint8_t value);

	void setMcpPortAPin(uint8_t pin, bool value);

	void setMcpPortBPin(uint8_t pin, bool value);

	// Configure MCP23017 pins and interrupts
	void configureMcp();

	// Scan I2C bus for devices
	void scanI2CBus();

	// Queue for communicating between ISR and task
	static QueueHandle_t myEventQueue;
};
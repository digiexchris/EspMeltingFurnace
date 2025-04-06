#pragma once

#include "hardware.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

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
	
	static GPIOManager *myInstance;

	// Last switch state to detect changes
	bool myLastSwitchState = false;
	bool myIsDoorOpen = true;

	// Interrupt handler task
	static void interruptHandlerTask(void *arg);
	TaskHandle_t myTaskHandle = nullptr;

	void processDoorSwitch();

	// Queue for communicating between ISR and task
	static QueueHandle_t myEventQueue;
};
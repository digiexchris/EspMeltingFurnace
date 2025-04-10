#pragma once

#include "hardware.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <unordered_map>

class GPIOManager
{
public:
	GPIOManager();

	static GPIOManager *GetInstance()
	{
		if (myInstance == nullptr)
		{
			myInstance = new GPIOManager();
		}
		return myInstance;
	}

	void setEmergencyRelay(bool value);
	bool isDoorOpen()
	{
		return myIsDoorOpen;
	};

private:
	static GPIOManager *myInstance;

	bool myIsDoorOpen = true;

	// pair of the timer and the previously valid state of the switch
	std::unordered_map<gpio_num_t, std::pair<TimerHandle_t, bool>> mySwitchState;

	static void processSwitch(void *arg);
	static void debounceSwitchCallback(TimerHandle_t xTimer);
};
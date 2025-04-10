#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <esp_log.h>

#include "max31856-espidf/max31856.hxx"
#include <AutoPID-for-ESP-IDF.h>

#include "SPIBus.hxx"
#include "uiTemp.hxx"

class TempController
{
public:
	TempController(TempUI *aTempUi, SPIBusManager *aBusManager);
	~TempController();

	MAX31856::Result GetLastResult()
	{
		return myLastResult;
	}

private:
	static constexpr int SSR_FULL_PWM = 1023;
	static constexpr int SSR_REDUCED_PWM = 512; // to be enabled once on 240v
	// static constexpr int SSR_REDUCED_PWM = SSR_FULL_PWM;
	static constexpr int SSR_OFF_PWM = 0;
	static constexpr gpio_num_t SSR_PIN = HEATER_SSR_PIN;
	static constexpr int SSR_REDUCED_PWM_UNDER = 400;
	static constexpr int SSR_BANG_BANG_WINDOW = 50;

	int SSR_CURRENT_MAX_PWM = SSR_REDUCED_PWM;
	int SSR_CURRENT_PWM = SSR_OFF_PWM;
	TempUI *myTempUi;
	SPIBusManager *mySpiBusManager;
	TempController *myInstance;
	QueueHandle_t myThermocoupleQueue;
	MAX31856::MAX31856 *myThermocouple;
	MAX31856::Result myLastResult;

	bool *myRelayState = nullptr; // dummy relay state for the AutoPIDRelay constructor, will be using the pulse width instead

	double *myLastCurrentTemp = nullptr;
	double *myLastSetTemp = nullptr;
	static void receiverTask(void *pvParameter);
	static void thermocoupleTask(void *pvParameter);
	static void pidTask(void *pvParameter);

	void initSSR();
	void initPID();
	void setSSRDutyCycle(int duty);

	AutoPIDRelay *myAutoPIDRelay = nullptr;
};
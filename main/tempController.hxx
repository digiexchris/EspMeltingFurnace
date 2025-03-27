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
	TempUI *myTempUi;
	SPIBusManager *mySpiBusManager;
	TempController *myInstance;
	QueueHandle_t myThermocoupleQueue;
	MAX31856::MAX31856 *myThermocouple;
	MAX31856::Result myLastResult;

	static void receiverTask(void *pvParameter);
	static void thermocoupleTask(void *pvParameter);
};
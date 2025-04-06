#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h" // Added for software PWM timer

#include <esp_log.h>

#include "max31856-espidf/max31856.hxx"
#include <AutoPID-for-ESP-IDF.h>

#include "SPIBus.hxx"
#include "modbus/Proto.hxx"

class TempController
{
public:
	TempController(SPIBusManager *aBusManager);
	~TempController();

	MAX31856::Result GetLastResult()
	{
		return myLastResult;
	}

private:
	static constexpr int SSR_FULL_PWM = 1023;
	static constexpr int SSR_REDUCED_PWM = 512;
	static constexpr int SSR_OFF_PWM = 0;
	static constexpr int SSR_REDUCED_PWM_UNDER = 400;
	static constexpr int SSR_BANG_BANG_WINDOW = 50;

	// Software PWM constants
	static constexpr int PWM_PERIOD_MS = 5000; // 5-second period

	int SSR_CURRENT_MAX_PWM = SSR_REDUCED_PWM;
	int SSR_CURRENT_PWM = SSR_OFF_PWM;
	SPIBusManager *mySpiBusManager;
	TempController *myInstance;
	QueueHandle_t myThermocoupleQueue;
	MAX31856::MAX31856 *myThermocouple;
	MAX31856::Result myLastResult;

	ErrorCode myErrorCode = ErrorCode::NO_ERROR;

	bool *myRelayState = nullptr; // dummy relay state for the AutoPIDRelay constructor, will be using the pulse width instead

	double *myCurrentTemp = nullptr;
	double *mySetTemp = nullptr;
	bool myIsEnabled = false;
	static void receiverTask(void *pvParameter);
	static void thermocoupleTask(void *pvParameter);
	static void pidTask(void *pvParameter);

	// Software PWM timer and callback
	TimerHandle_t mySoftPwmTimer = nullptr;
	static void softPwmTimerCallback(TimerHandle_t xTimer);
	void updateSoftPwm();

	void initSSR();
	void initPID();
	void setSSRDutyCycle(int duty);

	AutoPIDRelay *myAutoPIDRelay = nullptr;

	void SetError(ErrorCode error)
	{
		myErrorCode |= error;
	}

	void ClearError(ErrorCode error)
	{
		myErrorCode &= ~error;
	}

	bool IsError()
	{
		return myErrorCode != ErrorCode::NO_ERROR;
	}

	bool IsErrorSet(ErrorCode error)
	{
		return (myErrorCode & error) == error;
	}
};
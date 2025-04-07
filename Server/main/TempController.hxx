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
	struct Config
	{
		/*
		data.HEATER_REDUCE_PWM_UNDER = controller->GetReducePwmUnder();
	data.HEATER_REDUCED_PWM_VALUE = controller->GetReducedPwmValue();
	data.HEATER_PERIOD = controller->GetHeaterPeriod();
	data.P = controller->GetP();
	data.I = controller->GetI();
	data.D = controller->GetD();
	data.TARGET_TEMP = controller->GetTargetTemp();
	data.PID_WINDOW = controller->GetPidWindow();
	*/
		int P = 30;
		int I = 50;
		int D = 1;
		int SSR_REDUCED_PWM_UNDER = 400; // aka HEATER_REDUCE_PWM_UNDER
		int SSR_REDUCED_PWM_VALUE = 512; // aka HEATER_REDUCED_PWM_VALUE
		int SSR_FULL_PWM = 1023;		 // aka HEATER_FULL_PWM
		int SSR_OFF_PWM = 0;			 // aka HEATER_OFF_PWM
		int SSR_BANG_BANG_WINDOW = 50;	 // aka PID_WINDOW
		int PWM_PERIOD_MS = 5000;		 // aka HEATER_PERIOD
	};

	TempController(SPIBusManager *aBusManager);
	~TempController();

	MAX31856::Result GetLastResult()
	{
		return myLastResult;
	}

	void SetTemp(double temp)
	{
		*mySetTemp = temp;
	}

	static TempController *GetInstance()
	{
		return myInstance;
	}

	Config GetConfig()
	{
		return myConfig;
	}

	void SetConfig(Config config)
	{
		myConfig = config;

		// todo Save it
		// todo set the pid params
	}

	float GetTargetTemp()
	{
		return *mySetTemp;
	}

	void SetTargetTemp(double temp)
	{
		*mySetTemp = temp;
	}

	float GetCurrentTemp()
	{
		return *myCurrentTemp;
	}

	int GetPwmDutyCycle()
	{
		return SSR_CURRENT_PWM;
	}

private:
	Config myConfig;

	int SSR_CURRENT_MAX_PWM = 512;
	int SSR_CURRENT_PWM = 0;
	SPIBusManager *mySpiBusManager;
	static TempController *myInstance;
	QueueHandle_t myThermocoupleQueue;
	MAX31856::MAX31856 *myThermocouple;
	MAX31856::Result myLastResult;

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
};
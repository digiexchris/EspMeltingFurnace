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
#include "TempDevice.hxx"
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
		int SSR_REDUCED_PWM_UNDER = STARTUP_HEATING_RATE_UNDER_TEMP; // aka HEATER_REDUCE_PWM_UNDER
		int SSR_REDUCED_PWM_VALUE = 512;							 // aka HEATER_REDUCED_PWM_VALUE
		int SSR_FULL_PWM = 1023;									 // aka HEATER_FULL_PWM
		int SSR_OFF_PWM = 0;										 // aka HEATER_OFF_PWM
		int SSR_BANG_BANG_WINDOW = HEATING_BANG_BANG_WINDOW;		 // aka PID_WINDOW in AutoPid
		int PWM_PERIOD_MS = (1000.0f / LINE_FREQ * 10);				 // aka HEATER_PERIOD, 1/10th of the line frequency since a zero crossing SSR will only operate at line frequency. This gives us a resolution of approximately 6 steps of 16.666667 ms each.
		float HEATING_RATE_PER_SECOND = MAX_HEATING_RATE_PER_SECOND;
	};

	TempController(TempDevice *aTempDevice, SPIBusManager *aBusManager);
	~TempController();

	void SetTemp(double temp)
	{
		mySetTemp = temp;
	}

	static TempController *GetInstance()
	{
		return myInstance;
	}

	Config GetConfig()
	{
		return myConfig;
	}

	float GetInternalSetTemp()
	{
		return *myInternalSetTemp;
	}

	void SetConfig(Config config)
	{
		myConfig = config;

		// todo Save it
		// todo set the pid params
	}

	float GetTargetTemp()
	{
		return mySetTemp;
	}

	void SetTargetTemp(double temp)
	{
		mySetTemp = temp;
	}

	float GetCurrentTemp()
	{
		return *myCurrentTemp;
	}

	int GetPwmDutyCycle()
	{
		return SSR_CURRENT_PWM;
	}

	TempDevice *GetTempDevice()
	{
		return myTempDevice;
	}

	void SetHeatingRate(float rate)
	{
		myConfig.HEATING_RATE_PER_SECOND = rate;
	}

private:
	Config myConfig;
	TempDevice *myTempDevice;

	int SSR_CURRENT_MAX_PWM = 1023;
	int SSR_CURRENT_PWM = 0;
	SPIBusManager *mySpiBusManager;
	static TempController *myInstance;

	bool *myRelayState = nullptr; // dummy relay state for the AutoPIDRelay constructor, will be using the pulse width instead
	double *myCurrentTemp = nullptr;
	double mySetTemp = 0;				 // the user's requrested temp
	double *myInternalSetTemp = nullptr; // the temp as the target for the PID. This differs from the user's requested temp because this supports slowing the heating rate (ramp/soak)
	bool myIsEnabled = false;
	static void receiverTask(void *pvParameter);
	static void thermocoupleTask(void *pvParameter);
	static void pidTask(void *pvParameter);
	static void heatRateTask(void *pvParameter);

	// Software PWM timer and callback
	TimerHandle_t mySoftPwmTimer = nullptr;
	static void softPwmTimerCallback(TimerHandle_t xTimer);
	void updateSoftPwm();

	void initSSR();
	void initPID();
	void setSSRDutyCycle(int duty);

	AutoPIDRelay *myAutoPIDRelay = nullptr;
};
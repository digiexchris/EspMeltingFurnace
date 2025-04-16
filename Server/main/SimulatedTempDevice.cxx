#include "TempController.hxx"
#include "TempDevice.hxx"
#include "hardware.h"

SimulatedTempDevice::SimulatedTempDevice()
{
	myResult.thermocouple_c = 25.0;
	myResult.thermocouple_f = 77.0;
	myHighFaultThreshold = 1350.0;
	myLowFaultThreshold = 5.0;
	myResult.fault = TempFault::NONE;

	xTaskCreate(&TempTask, "CoolingTask", 2048 * 2, this, 5, NULL);
}

void SimulatedTempDevice::SetTemp(float celsius)
{
	myResult.thermocouple_c = celsius;
	myResult.thermocouple_f = (celsius * 9.0 / 5.0) + 32.0;

	if (myResult.thermocouple_c > myHighFaultThreshold)
	{
		myResult.fault = TempFault::TCHIGH;
	}
	else if (myResult.thermocouple_c < myLowFaultThreshold)
	{
		myResult.fault = TempFault::TCLOW;
	}
	else
	{
		myResult.fault = TempFault::NONE;
	}
}

// 0 to 1023 in percent per second
void SimulatedTempDevice::SetHeatingPowerPerSecond(float power)
{
	TempController *controller = TempController::GetInstance();
	TempController::Config config = controller->GetConfig();

	if (power < 0)
	{
		power = 0;
	}
	else if (power > config.SSR_FULL_PWM)
	{
		power = config.SSR_FULL_PWM;
	}

	myHeatingPowerPerSecond = power;
}

TempResult SimulatedTempDevice::GetResult()
{
	return myResult;
}

void SimulatedTempDevice::SetType(TempType type)
{
	myType = type;
}
void SimulatedTempDevice::SetTempFaultThresholds(float high, float low)
{
	myHighFaultThreshold = high;
	myLowFaultThreshold = low;

	if (myResult.thermocouple_c > myHighFaultThreshold)
	{
		myResult.fault = TempFault::TCHIGH;
	}
	else if (myResult.thermocouple_c < myLowFaultThreshold)
	{
		myResult.fault = TempFault::TCLOW;
	}
	else
	{
		myResult.fault = TempFault::NONE;
	}
}

void SimulatedTempDevice::TempTask(void *pvParameter)
{
	SimulatedTempDevice *instance = static_cast<SimulatedTempDevice *>(pvParameter);
	float currentTemp = 0;

	TempController::Config config;

	while (42)
	{
		if (instance == nullptr)
		{
			ESP_LOGE("Cooling Task", "instance is null");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		currentTemp = instance->GetTempC();

		// ambient temp management first
		if (currentTemp > AMBIENT_TEMP)
		{
			float newTemp = currentTemp - COOLING_RATE_PER_SECOND;
			if (newTemp < AMBIENT_TEMP)
			{
				newTemp = AMBIENT_TEMP;
			}
			instance->SetTemp(newTemp);
		}

		else if (currentTemp < AMBIENT_TEMP)
		{
			float newTemp = currentTemp + COOLING_RATE_PER_SECOND;
			if (newTemp > AMBIENT_TEMP)
			{
				newTemp = AMBIENT_TEMP;
			}
			instance->SetTemp(newTemp);
		}

		// now apply heater
		if (instance->myHeatingPowerPerSecond > 0)
		{

			float tempIncrease = instance->myHeatingPowerPerSecond / config.SSR_FULL_PWM * HEAT_RATE_PER_SECOND;
			float newTemp = currentTemp + (instance->myHeatingPowerPerSecond / 100.0f) * HEAT_RATE_PER_SECOND;
			if (newTemp > MAX_TEMP)
			{
				newTemp = MAX_TEMP;
			}
			instance->SetTemp(newTemp);
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
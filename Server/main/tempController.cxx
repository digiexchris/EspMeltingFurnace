#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

#include "max31856-espidf/max31856.hxx"

#include "tempController.hxx"

#include "driver/gpio.h"
#include "driver/ledc.h"

#include <esp_log.h>

#include "GPIO.hxx"

const char *TCTAG = "TempController";

bool isInRange(int value, int min, int max)
{
	return value > min && value < max;
}

TempController::TempController(SPIBusManager *aBusManager) : mySpiBusManager(aBusManager), myInstance(nullptr), myThermocoupleQueue(nullptr)
{
	ESP_LOGI(TCTAG, "Initialize");

	myInstance = this;

	assert(aBusManager != nullptr);

	mySpiBusManager->lock();
	myThermocouple = new MAX31856::MAX31856(SPI3_HOST, false);
	myThermocouple->AddDevice(MAX31856::ThermocoupleType::MAX31856_TCTYPE_K, MAX31856_SPI3_CS, 0);
	mySpiBusManager->unlock();

	myThermocoupleQueue = xQueueCreate(5, sizeof(struct MAX31856::Result));
	if (myThermocoupleQueue != NULL)
	{
		ESP_LOGI(TCTAG, "Created: thermocouple_queue");
	}
	else
	{
		ESP_LOGE(TCTAG, "Failed to Create: thermocouple_queue");
		assert(false);
	}

	// Allocate memory for data passed in and out of autopid
	myCurrentTemp = new double(0.0);
	mySetTemp = new double(0.0);
	myRelayState = new bool(false);

	initPID();

	xTaskCreate(&thermocoupleTask, "thermocouple_task", 2048, this, 6, NULL);
	xTaskCreate(&receiverTask, "receiver_task", 2048, this, 6, NULL);

	xTaskCreate(&pidTask, "pid_task", 2048, this, 6, NULL);
}

TempController::~TempController()
{
	// Clean up resources
	if (myThermocouple != nullptr)
	{
		delete myThermocouple;
	}
	if (myAutoPIDRelay != nullptr)
	{
		delete myAutoPIDRelay;
	}
	if (myRelayState != nullptr)
	{
		delete myRelayState;
	}
	if (myCurrentTemp != nullptr)
	{
		delete myCurrentTemp;
	}

	// Turn off SSR when destroying controller
	setSSRDutyCycle(SSR_OFF_PWM);
}

void TempController::thermocoupleTask(void *pvParameter)
{
	ESP_LOGI("task", "Created: receiver_task");
	TempController *instance = static_cast<TempController *>(pvParameter);

	MAX31856::Result result = {
		.coldjunction_c = 0,
		.coldjunction_f = 0,
		.thermocouple_c = 0,
		.thermocouple_f = 0,
		.fault = 0,
	};

	while (42)
	{
		instance->mySpiBusManager->lock();
		instance->myThermocouple->read(result, 0);
		instance->mySpiBusManager->unlock();
		xQueueSend(instance->myThermocoupleQueue, &result, (TickType_t)0);
		vTaskDelay(600 / portTICK_PERIOD_MS);
	}
}

void TempController::receiverTask(void *pvParameter)
{
	ESP_LOGI("task", "Created: thermocouple_task");
	TempController *instance = static_cast<TempController *>(pvParameter);

	MAX31856::Result result = {
		.coldjunction_c = 0,
		.coldjunction_f = 0,
		.thermocouple_c = 0,
		.thermocouple_f = 0,
		.fault = 0,
	};

	float lastTemp = 0;
	float lastColdTemp = 0;

	while (1)
	{
		xQueueReceive(instance->myThermocoupleQueue, &result, (TickType_t)(200 / portTICK_PERIOD_MS));

		if (result.thermocouple_c != lastTemp || result.coldjunction_c != lastColdTemp)
		{
			lastTemp = result.thermocouple_c;
			lastColdTemp = result.coldjunction_c;

			instance->myLastResult = result;
			*instance->myCurrentTemp = result.thermocouple_c;
			if (lastTemp < SSR_REDUCED_PWM_UNDER && instance->SSR_CURRENT_MAX_PWM != SSR_REDUCED_PWM)
			{
				instance->myAutoPIDRelay->setOutputRange(SSR_OFF_PWM, SSR_REDUCED_PWM);
				instance->SSR_CURRENT_MAX_PWM = SSR_REDUCED_PWM;
			}
			else
			{
				if (instance->SSR_CURRENT_MAX_PWM != SSR_FULL_PWM)
				{
					instance->myAutoPIDRelay->setOutputRange(SSR_OFF_PWM, SSR_FULL_PWM);
					instance->SSR_CURRENT_MAX_PWM = SSR_FULL_PWM;
				}
			}
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void TempController::pidTask(void *pvParameter)
{
	TempController *instance = static_cast<TempController *>(pvParameter);

	GPIOManager *gpio = GPIOManager::GetInstance();

	while (42)
	{
		if (instance == nullptr)
		{
			ESP_LOGE(TCTAG, "instance is null");
			abort();
		}

		if (instance->myCurrentTemp == nullptr)
		{
			ESP_LOGE(TCTAG, "myCurrentTemp is null");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		if (instance->mySetTemp == nullptr)
		{
			ESP_LOGE(TCTAG, "mySetTemp is null");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		if (instance->myRelayState == nullptr)
		{
			ESP_LOGE(TCTAG, "myRelayState is null");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		if (gpio == nullptr)
		{
			ESP_LOGE(TCTAG, "GPIOManager instance is null");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		// todo these need to be set in the error handling stuff. like, the temp loop checks for temp errors including this limit.
		// float upperLimit = std::min(*instance->mySetTemp + 100, 1350.0);
		bool doorOpen = gpio->isDoorOpen();

		if (instance->myErrorCode == ErrorCode::NO_ERROR)
		{

			gpio->setEmergencyRelay(false);

			if (instance->myIsEnabled && !doorOpen)
			{
				// reminder: the set point is a pointer to a variable in this dcflass, no need to manually call setters on the autopid to manage it.
				ESP_LOGI(TCTAG, "Running PID");
				instance->myAutoPIDRelay->run();

				int output = instance->myAutoPIDRelay->getPulseValue();

				if (output != instance->SSR_CURRENT_PWM)
				{
					instance->SSR_CURRENT_PWM = output;
					instance->setSSRDutyCycle(output);
				}
			}
			else
			{
				instance->myAutoPIDRelay->stop();
				instance->SSR_CURRENT_PWM = SSR_OFF_PWM;
				instance->setSSRDutyCycle(SSR_OFF_PWM);
			}
		}
		else
		{
			instance->myAutoPIDRelay->stop();
			instance->SSR_CURRENT_PWM = SSR_OFF_PWM;
			instance->setSSRDutyCycle(SSR_OFF_PWM);
			gpio->setEmergencyRelay(true);
		}

		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

void TempController::initPID()
{
	// Initialize PID controller
	myAutoPIDRelay = new AutoPIDRelay(myCurrentTemp, mySetTemp, myRelayState, 2000.0, 30.0, 50.0, 0.1);
	myAutoPIDRelay->setBangBang(SSR_BANG_BANG_WINDOW);
	myAutoPIDRelay->setTimeStep(1000);
	myAutoPIDRelay->setOutputRange(SSR_OFF_PWM, SSR_FULL_PWM);

	// Initialize SSR
	initSSR();
}

void TempController::initSSR()
{
	// Configure SSR pin as output
	gpio_config_t io_conf = {
		.pin_bit_mask = (1ULL << HEATER_SSR_PIN),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE};

	gpio_config(&io_conf);

	// Initial state is off
	gpio_set_level(HEATER_SSR_PIN, 0);

	// Create software PWM timer (5-second period)
	mySoftPwmTimer = xTimerCreate(
		"SoftPWM_Timer",
		pdMS_TO_TICKS(PWM_PERIOD_MS),
		pdTRUE, // Auto-reload timer
		this,	// Timer ID
		softPwmTimerCallback);

	if (mySoftPwmTimer == nullptr)
	{
		ESP_LOGE(TCTAG, "Failed to create software PWM timer");
		assert(false);
	}

	// Start the timer
	if (xTimerStart(mySoftPwmTimer, 0) != pdPASS)
	{
		ESP_LOGE(TCTAG, "Failed to start software PWM timer");
		assert(false);
	}

	ESP_LOGI(TCTAG, "Software PWM initialized with period %d ms", PWM_PERIOD_MS);
}

void TempController::setSSRDutyCycle(int duty)
{
	// Validate duty cycle is within range
	if (duty < SSR_OFF_PWM || duty > SSR_CURRENT_MAX_PWM)
	{
		ESP_LOGE(TCTAG, "Duty cycle out of range: %d", duty);
		SSR_CURRENT_PWM = SSR_OFF_PWM;

		// Immediately turn off SSR for safety
		gpio_set_level(HEATER_SSR_PIN, 0);
		return;
	}

	// Store the new duty cycle value
	// The software PWM logic in updateSoftPwm() will use this value
	// on the next PWM cycle
	SSR_CURRENT_PWM = duty;

	ESP_LOGD(TCTAG, "SSR duty cycle set to %d/%d", duty, SSR_FULL_PWM);
}

// Static callback function for the software PWM timer
void TempController::softPwmTimerCallback(TimerHandle_t xTimer)
{
	// Get instance pointer from the timer ID
	TempController *instance = static_cast<TempController *>(pvTimerGetTimerID(xTimer));
	if (instance == nullptr)
	{
		ESP_LOGE(TCTAG, "Invalid timer ID");
		return;
	}

	instance->updateSoftPwm();
}

// Called each time the PWM timer fires to update the PWM output
void TempController::updateSoftPwm()
{
	static uint32_t cycleStartTime = 0;
	static int currentDuty = 0;

	uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

	// At the start of each PWM cycle
	if (currentTime - cycleStartTime >= PWM_PERIOD_MS)
	{
		cycleStartTime = currentTime;
		currentDuty = SSR_CURRENT_PWM;

		// Calculate on-time based on duty cycle (0-1023)
		int onTimeMs = (currentDuty * PWM_PERIOD_MS) / SSR_FULL_PWM;

		if (onTimeMs > 0)
		{
			// Turn on the SSR
			gpio_set_level(HEATER_SSR_PIN, 1);

			// If not full duty cycle, schedule the off time
			if (onTimeMs < PWM_PERIOD_MS)
			{
				// Schedule timer to fire after on-time for turn-off
				xTimerChangePeriod(mySoftPwmTimer, pdMS_TO_TICKS(onTimeMs), 0);
				return;
			}
		}
		else
		{
			// Zero duty cycle - keep SSR off
			gpio_set_level(HEATER_SSR_PIN, 0);
		}
	}
	else
	{
		// This is the off-time portion of the cycle
		gpio_set_level(HEATER_SSR_PIN, 0);

		// Reset timer to full period for next cycle
		xTimerChangePeriod(mySoftPwmTimer, pdMS_TO_TICKS(PWM_PERIOD_MS - (currentTime - cycleStartTime)), 0);
	}
}
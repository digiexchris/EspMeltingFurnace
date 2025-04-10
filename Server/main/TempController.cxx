#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max31856-espidf/max31856.hxx"
#include "sdkconfig.h"
#include <esp_log.h>
#include <stdio.h>

#include "GPIO.hxx"
#include "State.hxx"
#include "TempController.hxx"

#include "TempDevice.hxx"

const char *TCTAG = "TempController";

bool isInRange(int value, int min, int max)
{
	return value > min && value < max;
}

TempController *TempController::myInstance = nullptr;

TempController::TempController(TempDevice *aTempDevice, SPIBusManager *aBusManager) : myTempDevice(aTempDevice), mySpiBusManager(aBusManager)
{
	ESP_LOGI(TCTAG, "Initialize");

	myInstance = this;

	// Allocate memory for data passed in and out of autopid
	myCurrentTemp = new double(0.0);
	mySetTemp = new double(0.0);
	myRelayState = new bool(false);

	initPID();

	xTaskCreate(&pidTask, "pid_task", 2048 * 2, this, 6, NULL);
}

TempController::~TempController()
{
	// Clean up resources

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
	setSSRDutyCycle(myConfig.SSR_OFF_PWM);
}

void TempController::pidTask(void *pvParameter)
{
	TempController *instance = static_cast<TempController *>(pvParameter);
	TempDevice *thermocouple = instance->myTempDevice;

	GPIOManager *gpio = GPIOManager::GetInstance();

	State *state = State::GetInstance();

	TempResult result;

	if (instance == nullptr)
	{
		ESP_LOGE(TCTAG, "instance is null");
		abort();
	}

	while (42)
	{
		if (gpio == nullptr)
		{
			ESP_LOGE(TCTAG, "GPIOManager instance is null");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			State *state = State::GetInstance();
			continue;
		}

		result = thermocouple->GetResult();

		float highLimit = std::min<float>((float)*instance->mySetTemp + 50, MAX_TEMP);

		if (result.thermocouple_c > highLimit)
		{
			state->SetError(ErrorCode::TEMP_RANGE_HIGH);
		}
		else
		{
			if (state->IsErrorSet(ErrorCode::TEMP_RANGE_HIGH))
			{
				state->ClearError(ErrorCode::TEMP_RANGE_HIGH);
			}
		}

		if (result.thermocouple_c < MIN_TEMP)
		{
			state->SetError(ErrorCode::TEMP_RANGE_LOW);
		}
		else
		{
			if (state->IsErrorSet(ErrorCode::TEMP_RANGE_LOW))
			{
				state->ClearError(ErrorCode::TEMP_RANGE_LOW);
			}
		}

		*instance->myCurrentTemp = result.thermocouple_c;
		if (*instance->myCurrentTemp < instance->myConfig.SSR_REDUCED_PWM_UNDER && instance->SSR_CURRENT_MAX_PWM != instance->myConfig.SSR_REDUCED_PWM_VALUE)
		{
			instance->myAutoPIDRelay->setOutputRange(instance->myConfig.SSR_OFF_PWM, instance->myConfig.SSR_REDUCED_PWM_VALUE);
			instance->SSR_CURRENT_MAX_PWM = instance->myConfig.SSR_REDUCED_PWM_VALUE;
		}
		else
		{
			if (instance->SSR_CURRENT_MAX_PWM != instance->myConfig.SSR_FULL_PWM)
			{
				instance->myAutoPIDRelay->setOutputRange(instance->myConfig.SSR_OFF_PWM, instance->myConfig.SSR_FULL_PWM);
				instance->SSR_CURRENT_MAX_PWM = instance->myConfig.SSR_FULL_PWM;
			}
		}

		if (!state->HasError() && state->IsEnabled())
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
			instance->SSR_CURRENT_PWM = instance->myConfig.SSR_OFF_PWM;
			instance->setSSRDutyCycle(instance->myConfig.SSR_OFF_PWM);

			if (state->HasError())
			{
				gpio->setEmergencyRelay(true);
			}
		}

		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

void TempController::initPID()
{
	myAutoPIDRelay = new AutoPIDRelay(myCurrentTemp, mySetTemp, myRelayState, myConfig.PWM_PERIOD_MS, myConfig.P, myConfig.D, myConfig.I);
	myAutoPIDRelay->setBangBang(myConfig.SSR_BANG_BANG_WINDOW);
	myAutoPIDRelay->setTimeStep(1000);
	myAutoPIDRelay->setOutputRange(myConfig.SSR_OFF_PWM, myConfig.SSR_FULL_PWM);

	initSSR();
}

void TempController::initSSR()
{
	gpio_config_t io_conf = {
		.pin_bit_mask = (1ULL << HEATER_SSR_PIN),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE};

	gpio_config(&io_conf);
	gpio_set_level(HEATER_SSR_PIN, 0);
	mySoftPwmTimer = xTimerCreate(
		"SoftPWM_Timer",
		pdMS_TO_TICKS(myConfig.PWM_PERIOD_MS),
		pdTRUE, // Auto-reload timer
		this,	// Timer ID
		softPwmTimerCallback);

	if (mySoftPwmTimer == nullptr)
	{
		ESP_LOGE(TCTAG, "Failed to create software PWM timer");
		assert(false);
	}

	if (xTimerStart(mySoftPwmTimer, 0) != pdPASS)
	{
		ESP_LOGE(TCTAG, "Failed to start software PWM timer");
		assert(false);
	}

	ESP_LOGI(TCTAG, "Software PWM initialized with period %d ms", myConfig.PWM_PERIOD_MS);
}

void TempController::setSSRDutyCycle(int duty)
{
	// Validate duty cycle is within range
	if (duty < myConfig.SSR_OFF_PWM || duty > SSR_CURRENT_MAX_PWM)
	{
		ESP_LOGE(TCTAG, "Duty cycle out of range: %d", duty);
		SSR_CURRENT_PWM = myConfig.SSR_OFF_PWM;

		// Immediately turn off SSR for safety
		gpio_set_level(HEATER_SSR_PIN, 0);
		return;
	}
	SSR_CURRENT_PWM = duty;

	ESP_LOGD(TCTAG, "SSR duty cycle set to %d/%d", duty, myConfig.SSR_FULL_PWM);
}

void TempController::softPwmTimerCallback(TimerHandle_t xTimer)
{
	TempController *instance = static_cast<TempController *>(pvTimerGetTimerID(xTimer));
	if (instance == nullptr)
	{
		ESP_LOGE(TCTAG, "Invalid timer ID");
		return;
	}

	instance->updateSoftPwm();
}

void TempController::updateSoftPwm()
{
	static uint32_t cycleStartTime = 0;
	static int currentDuty = 0;

	uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

	// At the start of each PWM cycle
	if (currentTime - cycleStartTime >= myConfig.PWM_PERIOD_MS)
	{
		cycleStartTime = currentTime;
		currentDuty = SSR_CURRENT_PWM;

		// Calculate on-time based on duty cycle (0-1023)
		int onTimeMs = (currentDuty * myConfig.PWM_PERIOD_MS) / myConfig.SSR_FULL_PWM;

		if (onTimeMs > 0)
		{
			// Turn on the SSR
			gpio_set_level(HEATER_SSR_PIN, 1);

			// If not full duty cycle, schedule the off time
			if (onTimeMs < myConfig.PWM_PERIOD_MS)
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
		xTimerChangePeriod(mySoftPwmTimer, pdMS_TO_TICKS(myConfig.PWM_PERIOD_MS - (currentTime - cycleStartTime)), 0);
	}
}
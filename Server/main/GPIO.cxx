#include "GPIO.hxx"
#include "hardware.h"
#include <esp_log.h>

#include "hardware.h"

#include "State.hxx"

static const char *GPIOTAG = "GPIO";

GPIOManager *GPIOManager::myInstance = nullptr;

GPIOManager::GPIOManager()
{
	myInstance = this;
	ESP_LOGI(GPIOTAG, "Initializing GPIO Manager");

	gpio_config_t relayConf = {
		.pin_bit_mask = (1ULL << EMERGENCY_RELAY_PIN),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&relayConf);
	gpio_set_level(EMERGENCY_RELAY_PIN, EMERGENCY_RELAY_ON);

	gpio_config_t doorConf = {
		.pin_bit_mask = (1ULL << DOOR_SWITCH_PIN),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_ANYEDGE,
	};
	gpio_config(&doorConf);

	gpio_config_t enableConf = {
		.pin_bit_mask = (1ULL << ENABLE_SWITCH_PIN),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_ANYEDGE,
	};
	gpio_config(&enableConf);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(DOOR_SWITCH_PIN, processSwitch, (void *)DOOR_SWITCH_PIN);
	gpio_isr_handler_add(ENABLE_SWITCH_PIN, processSwitch, (void *)ENABLE_SWITCH_PIN);
}

void GPIOManager::processSwitch(void *arg)
{
	GPIOManager *gpio = GPIOManager::GetInstance();

	gpio_num_t pin = static_cast<gpio_num_t>(reinterpret_cast<uintptr_t>(arg));

	gpio->mySwitchState[pin].second = gpio_get_level(pin);

	if (gpio->mySwitchState[pin].first == nullptr)
	{
		gpio->mySwitchState[pin].first = xTimerCreate("DebounceTimer", pdMS_TO_TICKS(DEBOUNCE_DELAY_MS), pdFALSE, (void *)pin, debounceSwitchCallback);
	}
	else
	{
		xTimerChangePeriod(gpio->mySwitchState[pin].first, pdMS_TO_TICKS(DEBOUNCE_DELAY_MS), 0);
	}

	if (!xTimerIsTimerActive(gpio->mySwitchState[pin].first))
	{
		xTimerStart(gpio->mySwitchState[pin].first, 0);
	}
}

void GPIOManager::debounceSwitchCallback(TimerHandle_t xTimer)
{
	gpio_num_t pin = static_cast<gpio_num_t>(reinterpret_cast<uintptr_t>(pvTimerGetTimerID(xTimer)));

	GPIOManager *gpio = GPIOManager::GetInstance();

	bool level = gpio_get_level(pin);
	if (level != gpio->mySwitchState[pin].second)
	{
		gpio->mySwitchState[pin].second = level;
		State *state = State::GetInstance();

		switch (pin)
		{
		case DOOR_SWITCH_PIN:

			if (level)
			{
				state->QueueEvent(EventType::DOOR_OPEN);
			}
			else
			{
				state->QueueEvent(EventType::DOOR_CLOSED);
			}
			break;
		case ENABLE_SWITCH_PIN:
			if (level)
			{
				state->QueueEvent(EventType::ENABLE);
			}
			else
			{
				state->QueueEvent(EventType::DISABLE);
			}
			break;
		default:
			break;
		}
	}
}

void GPIOManager::setEmergencyRelay(bool value)
{
	if (value)
	{
		gpio_set_level(EMERGENCY_RELAY_PIN, EMERGENCY_RELAY_ON);
	}
	else
	{
		gpio_set_level(EMERGENCY_RELAY_PIN, !EMERGENCY_RELAY_ON);
	}
}
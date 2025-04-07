
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "GPIO.hxx"
#include "State.hxx"
#include "TempController.hxx"

State *State::myInstance = nullptr;

State::State()
{
	myInstance = this;
	ESP_LOGI("State", "Initializing State Manager");

	myEventQueue = xQueueCreate(100, sizeof(Event));
	if (myEventQueue == nullptr)
	{
		ESP_LOGE("State", "Failed to create event queue");
		return;
	}

	xTaskCreate(checkForErrorTask, "checkForErrorTask", 2048, this, 10, nullptr);
}

void State::receiverTask(void *arg)
{
	State *state = static_cast<State *>(arg);
	TempController *tempController = TempController::GetInstance();
	Event event;

	while (true)
	{
		if (xQueueReceive(state->myEventQueue, &event, portMAX_DELAY) == pdTRUE)
		{
			switch (event.type)
			{
			case EventType::DOOR_OPEN:
				state->SetError(ErrorCode::DOOR_OPEN);
				break;
			case EventType::DOOR_CLOSED:
				state->ClearError(ErrorCode::DOOR_OPEN);
				break;
			case EventType::ENABLE:
				state->SetEnabled(true);
				break;
			case EventType::DISABLE:
				state->SetEnabled(false);
				break;
			case EventType::SET_TEMP:
			{
				SetTempEvent *setTempEvent = static_cast<SetTempEvent *>(&event);
				tempController->SetTemp(setTempEvent->temperature);
				break;
			}
			break;
			default:
				break;
			}
		}
	}
}

void State::checkForErrorTask(void *arg)
{
	State *state = static_cast<State *>(arg);
	GPIOManager *gpio = GPIOManager::GetInstance();
	while (true)
	{
		if (state->HasError())
		{
			gpio->setEmergencyRelay(true);
		}
		else
		{
			gpio->setEmergencyRelay(false);
		}
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}
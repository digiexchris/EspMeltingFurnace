#pragma once

#include "Errors.hxx"

enum class EventType
{
	NONE,
	DOOR_OPEN,
	DOOR_CLOSED,
	SET_TEMP,
	ENABLE,
	DISABLE
};

struct Event
{
	EventType type;
	Event(EventType eventType) : type(eventType) {}
	Event() : type(EventType::NONE) {}
};

struct SetTempEvent : public Event
{
	SetTempEvent(float temp) : Event(EventType::SET_TEMP), temperature(temp) {}
	float temperature;
};

class State
{
public:
	State();

	static State *GetInstance()
	{
		if (!myInstance)
		{
			myInstance = new State();
		}
		return myInstance;
	}

	void SetError(ErrorCode error)
	{
		myErrorCode |= error;
	}

	void ClearError(ErrorCode error)
	{
		myErrorCode &= ~error;
	}

	bool HasError()
	{
		return myErrorCode != ErrorCode::NO_ERROR;
	}

	bool IsErrorSet(ErrorCode error)
	{
		return (myErrorCode & error) == error;
	}

	ErrorCode GetError()
	{
		return myErrorCode;
	}

	bool IsEnabled()
	{
		return myIsEnabled;
	}

	void SetEnabled(bool enabled)
	{
		myIsEnabled = enabled;
	}

	void QueueEvent(Event event)
	{
		assert(myEventQueue != nullptr);
		xQueueSend(myEventQueue, &event, portMAX_DELAY);
	}

private:
	QueueHandle_t myEventQueue = nullptr;
	static State *myInstance;
	bool myIsEnabled = false;
	ErrorCode myErrorCode = ErrorCode::NO_ERROR;

	static void checkForErrorTask(void *arg);
	static void receiverTask(void *pvParameter);
};
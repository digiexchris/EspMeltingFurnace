#pragma once

#include <cstdint>
#include <driver/gpio.h>

#include "SPIBus.hxx"

#include "max31856-espidf/max31856.hxx"

enum class TempType : uint8_t
{
	TCTYPE_B = 0b0000,
	TCTYPE_E = 0b0001,
	TCTYPE_J = 0b0010,
	TCTYPE_K = 0b0011,
	TCTYPE_N = 0b0100,
	TCTYPE_R = 0b0101,
	TCTYPE_S = 0b0110,
	TCTYPE_T = 0b0111,
};

struct TempResult
{
	float thermocouple_c;
	float thermocouple_f;
	uint8_t fault;

	bool operator==(const TempResult &other) const
	{
		return thermocouple_c == other.thermocouple_c &&
			   thermocouple_f == other.thermocouple_f &&
			   fault == other.fault;
	}

	bool operator==(const MAX31856::Result &other) const
	{
		return thermocouple_c == other.thermocouple_c &&
			   thermocouple_f == other.thermocouple_f &&
			   fault == other.fault;
	}

	TempResult &operator=(const TempResult &other)
	{
		thermocouple_c = other.thermocouple_c;
		thermocouple_f = other.thermocouple_f;
		fault = other.fault;
		return *this;
	}

	TempResult &operator=(const MAX31856::Result &other)
	{
		thermocouple_c = other.thermocouple_c;
		thermocouple_f = other.thermocouple_f;
		fault = other.fault;
		return *this;
	}
};

class TempDevice
{
public:
	virtual TempResult GetResult() = 0;
	virtual void SetType(TempType type) = 0;
	virtual void SetTempFaultThresholds(float high, float low) = 0;
};

class MAX31856TempDevice : public TempDevice
{
public:
	MAX31856TempDevice(SPIBusManager *aBusManager, gpio_num_t csPin);

	TempResult GetResult() override;
	void SetType(TempType type) override;
	void SetTempFaultThresholds(float high, float low) override;

private:
	QueueHandle_t myThermocoupleQueue;
	MAX31856::MAX31856 *myThermocouple;
	MAX31856::Result myLastResult;

	TempResult myTempResult;
	void thermocoupleTask(void *pvParameter);
	void receiverTask(void *pvParameter);
};

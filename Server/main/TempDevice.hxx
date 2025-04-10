#pragma once

#include <cstdint>
#include <driver/gpio.h>

#include "SPIBus.hxx"

#include "max31856-espidf/max31856.hxx"

#include <atomic>

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

enum class TempFault : uint8_t
{
	NONE = 0x00,	// No Fault
	OPEN = 0x01,	// Open Circuit
	OVUV = 0x02,	// Over/Under Voltage
	TCLOW = 0x04,	// Thermocouple Low
	TCHIGH = 0x08,	// Thermocouple High
	CJLOW = 0x10,	// Cold Junction Low
	CJHIGH = 0x20,	// Cold Junction High
	TCRANGE = 0x40, // Thermocouple Out of Range
	CJRANGE = 0x80	// Cold Junction Out of Range
};

struct TempResult
{
	float thermocouple_c;
	float thermocouple_f;
	TempFault fault;

	TempResult() : thermocouple_c(0.0f), thermocouple_f(0.0f), fault(TempFault::NONE) {}

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
			   fault == static_cast<TempFault>(other.fault);
	}

	TempResult(const TempResult &other) = default;
	TempResult &operator=(const TempResult &other) = default;

	TempResult &operator=(const MAX31856::Result &other)
	{
		thermocouple_c = other.thermocouple_c;
		thermocouple_f = other.thermocouple_f;
		fault = static_cast<TempFault>(other.fault);
		return *this;
	}

	TempResult &operator=(const MAX31856::Result *other)
	{
		if (other != nullptr)
		{
			thermocouple_c = other->thermocouple_c;
			thermocouple_f = other->thermocouple_f;
			fault = static_cast<TempFault>(other->fault);
		}
		return *this;
	}
};

class AtomicTempResult
{
private:
	std::atomic<TempResult> value;

public:
	AtomicTempResult() : value(TempResult{}) {}

	AtomicTempResult &operator=(const TempResult &other)
	{
		value.store(other, std::memory_order_release);
		return *this;
	}

	AtomicTempResult &operator=(const MAX31856::Result &other)
	{
		TempResult temp;
		temp = other;
		value.store(temp, std::memory_order_release);
		return *this;
	}

	AtomicTempResult &operator=(const MAX31856::Result *other)
	{
		TempResult temp;
		temp = other;
		value.store(temp, std::memory_order_release);
		return *this;
	}

	operator TempResult() const
	{
		return value.load(std::memory_order_acquire);
	}
};

class TempDevice
{
public:
	virtual TempResult GetResult() = 0;
	virtual void SetType(TempType type) = 0;
	virtual void SetTempFaultThresholds(float high, float low) = 0;
};

class SimulatedTempDevice : public TempDevice
{
public:
	SimulatedTempDevice();
	TempResult GetResult();
	void SetType(TempType type);
	void SetTempFaultThresholds(float high, float low);
	void SetTemp(float celsius);

private:
	TempResult myResult;
	TempType myType;
	float myHighFaultThreshold;
	float myLowFaultThreshold;
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
	gpio_num_t myCsPin;
	SPIBusManager *mySpiBusManager;

	AtomicTempResult myTempResult;
	static void thermocoupleTask(void *pvParameter);
};

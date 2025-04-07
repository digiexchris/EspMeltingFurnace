#pragma once

#include "hardware.h"
#include <array>
#include <pl_modbus.h>
#include <pl_uart.h>
#include <utility>

#pragma pack(push, 1)
struct Coils
{
	uint8_t ENABLE : 1;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DiscreteInputs
{
	uint8_t ERROR : 1;
};
#pragma pack(pop)

struct HoldingRegisters
{
	uint16_t HEATER_REDUCE_PWM_UNDER;
	uint16_t HEATER_REDUCED_PWM_VALUE; // in uint16_t percent. 0 - 100
	uint16_t HEATER_PERIOD;			   // in seconds
	uint16_t P;
	uint16_t I;
	uint16_t D;
	uint16_t TARGET_TEMP;
	uint16_t PID_WINDOW; // Outside of this window, PID will not be active and will use bang-bang

	static constexpr uint16_t COUNT = 8;
};

struct InputRegisters
{
	uint16_t HEATER_PWM_DUTY_CYCLE;
	uint16_t CURRENT_TEMP;
	uint16_t ERROR_CODE;
	static constexpr uint16_t COUNT = 3;
};

#pragma once

#include "hardware.h"
#include <array>
#include <pl_modbus.h>
#include <pl_uart.h>
#include <utility>

enum class ErrorCode : uint8_t
{
	NO_ERROR = 0x00,
	THERMOCOUPLE_ERROR = 0x01,
	TEMP_RANGE_HIGH = 0x02,
	TEMP_RANGE_LOW = 0x04,
	ESTOP = 0x02,
	NO_CURRENT = 0x04,
};

inline ErrorCode operator|(ErrorCode lhs, ErrorCode rhs)
{
	return static_cast<ErrorCode>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline ErrorCode operator&(ErrorCode lhs, ErrorCode rhs)
{
	return static_cast<ErrorCode>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline ErrorCode &operator|=(ErrorCode &lhs, ErrorCode rhs)
{
	lhs = static_cast<ErrorCode>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
	return lhs;
}

inline bool operator==(ErrorCode lhs, ErrorCode rhs)
{
	return static_cast<uint8_t>(lhs) == static_cast<uint8_t>(rhs);
}

inline bool operator!=(ErrorCode lhs, ErrorCode rhs)
{
	return static_cast<uint8_t>(lhs) != static_cast<uint8_t>(rhs);
}

inline ErrorCode operator~(ErrorCode code)
{
	return static_cast<ErrorCode>(~static_cast<uint8_t>(code));
}

inline ErrorCode &operator&=(ErrorCode &lhs, ErrorCode rhs)
{
	lhs = static_cast<ErrorCode>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
	return lhs;
}

enum class CoilMask : uint8_t
{
	ENABLE = 0x01, // 0b00000010
};

enum class DiscreteInputMask : uint8_t
{
	ERROR = 0x01,			// 0b00000001
	DOOR_OPEN = 0x04,		// 0b00000100
	EMERGENCY_RELAY = 0x08, // 0b00001000
	MODE = 0x10,			// 0b00010000
};

enum class HoldingRegister
{
	HEATER_REDUCE_PWM_UNDER,
	HEATER_REDUCED_PWM_VALUE, // in uint16_t percent. 0 - 100
	HEATER_PERIOD,			  // in seconds
	P,
	I,
	D,
	TARGET_TEMP,
	PID_WINDOW, // Outside of this window, PID will not be active and will use bang-bang
	NUM_HOLDING_REGISTERS,
};

enum class InputRegister
{
	HEATER_PWM_DUTY_CYCLE = 0,
	CURRENT_TEMP,
	ERROR_CODE,
	NUM_INPUT_REGISTERS,
};

using Coils = uint8_t;
using DiscreteInputs = uint8_t;
using HoldingRegisters = uint16_t[static_cast<size_t>(HoldingRegister::NUM_HOLDING_REGISTERS)];
using InputRegisters = uint16_t[static_cast<size_t>(InputRegister::NUM_INPUT_REGISTERS)];

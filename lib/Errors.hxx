#pragma once

#include <cstdint>

enum class ErrorCode : uint8_t
{
	NO_ERROR = 0x00,
	THERMOCOUPLE_ERROR = 0x01,
	TEMP_RANGE_HIGH = 0x02,
	TEMP_RANGE_LOW = 0x04,
	ESTOP = 0x02,
	NO_CURRENT = 0x04,
	DOOR_OPEN = 0x08,
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
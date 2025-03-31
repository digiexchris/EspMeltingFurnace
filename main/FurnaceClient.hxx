#pragma once

#include "hardware.h"
#include <array>
#include <pl_modbus.h>
#include <pl_uart.h>
#include <utility>

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

class FurnaceClient
{

public:
	FurnaceClient();
	FurnaceClient *GetInstance();

	uint16_t GetCurrentPWMDutyCycle(); // input register
	uint16_t GetCurrentTemp();		   // input register
	uint16_t GetTargetTemp();		   // holding register
	uint16_t GetErrorCode();		   // input register
	bool GetMode();					   // discrete input
	bool HasError();				   // discrete input
	bool IsDoorOpen();				   // discrete input
	bool IsEmergencyRelayActive();	   // discrete input
	bool SetConfig(uint16_t p, uint16_t i, uint16_t d, uint16_t period, uint16_t pidWindow, uint16_t reducedPwmValue, uint16_t reducedPwmUnder);
	bool SetTargetTemp(uint16_t targetTemp);
	bool EnableHeating();
	bool DisableHeating();
	bool IsHeatingEnabled();

private:
	bool SetCoil(CoilMask mask, bool value);
	bool IsDiscreteInputSet(DiscreteInputMask mask);
	uint16_t GetInputRegister(InputRegister reg);
	uint16_t GetHoldingRegister(HoldingRegister reg);
	void ReadTask(void *pvParameter);
	bool ReadCoils();
	bool ReadHoldingRegisters();
	bool ReadDiscreteInputs();
	bool ReadInputRegisters();
	bool WriteHoldingRegisters();

	FurnaceClient *myInstance = nullptr;
	std::shared_ptr<PL::Uart> myUart;
	PL::ModbusClient *myClient;
	Coils myCoils = 0;
	HoldingRegisters myHeaterConfiguration;
	DiscreteInputs myDiscreteInputs = 0;
	InputRegisters myInputRegisters;
	bool hasReadError = false;
};
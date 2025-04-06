#pragma once

#include "Proto.hxx"

class Client
{

public:
	Client();
	static Client *GetInstance();

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

	static Client *myInstance;
	std::shared_ptr<PL::Uart> myUart;
	PL::ModbusClient *myClient;
	Coils myCoils = 0;
	HoldingRegisters myHeaterConfiguration;
	DiscreteInputs myDiscreteInputs = 0;
	InputRegisters myInputRegisters;
	bool hasReadError = false;
};
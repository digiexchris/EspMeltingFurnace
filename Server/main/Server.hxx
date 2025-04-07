#pragma once

#include "modbus/Proto.hxx"
#include <esp_err.h>
#include <pl_modbus.h>
#include <pl_uart.h>

// this should be kept up to date with modbus/Proto.hxx so the client has the same structure
class DynamicDiscreteInputs : public PL::ModbusMemoryArea
{
public:
	esp_err_t OnRead() override;

private:
	DiscreteInputs data;
};

class DynamicCoils : public PL::ModbusMemoryArea
{
public:
	esp_err_t OnRead() override;
	esp_err_t OnWrite() override;

private:
	Coils data;
};

class DynamicHoldingRegisters : public PL::ModbusMemoryArea
{
public:
	esp_err_t OnRead() override;
	esp_err_t OnWrite() override;

private:
	HoldingRegisters data;
};

class DynamicInputRegisters : public PL::ModbusMemoryArea
{
public:
	esp_err_t OnRead() override;
	esp_err_t OnWrite() override;

private:
	InputRegisters data;
};

class Server
{
public:
	Server();
	static Server *GetInstance()
	{
		if (myInstance == nullptr)
		{
			myInstance = new Server();
		}
		return myInstance;
	}

private:
	static Server *myInstance;
	std::shared_ptr<PL::Uart> myUart;
	std::shared_ptr<PL::ModbusServer> myModbusServer;
	std::shared_ptr<DynamicHoldingRegisters> myHoldingRegisters;
	std::shared_ptr<DynamicInputRegisters> myInputRegisters;
	std::shared_ptr<DynamicCoils> myCoils;					 // sent as a single byte as a bitmask
	std::shared_ptr<DynamicDiscreteInputs> myDiscreteInputs; // sent as a single byte as a bitmask
};
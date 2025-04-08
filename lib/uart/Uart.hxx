#pragma once

#include <pl_uart.h>

class UARTManager
{
public:
	UARTManager(uart_port_t aPort = UART_NUM_0);

	static UARTManager *GetInstance()
	{
		if (myInstance == nullptr)
		{
			myInstance = new UARTManager();
		}
		return myInstance;
	}

	std::shared_ptr<PL::Uart> GetUart()
	{
		return myUart;
	}

private:
	static UARTManager *myInstance;

	std::shared_ptr<PL::Uart> myUart;
};
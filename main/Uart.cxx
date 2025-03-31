#include "Uart.hxx"

#include <esp_log.h>

#include <pl_uart.h>

#include <utility>

UARTManager *UARTManager::myInstance = nullptr;

UARTManager::UARTManager()
{
	myUart = std::make_shared<PL::Uart>(UART_NUM_0);

	myUart->Initialize();
	myUart->SetBaudRate(115200);
	myUart->SetDataBits(8);
	myUart->SetParity(PL::UartParity::even);
	myUart->SetStopBits(PL::UartStopBits::one);
	myUart->SetFlowControl(PL::UartFlowControl::none);
	myUart->Enable();
}
#include "Server.hxx"
#include "State.hxx"
#include "TempController.hxx"
#include "hardware.h"
#include "modbus/Proto.hxx"
#include "uart/Uart.hxx"

#include "driver/uart.h"
#include "pl_uart.h"

#include <esp_log.h>
#include <memory>

Server *Server::myInstance = nullptr;

static const char *ServerTAG = "Server";

Server::Server()
{
	myUart = std::make_shared<PL::Uart>(MODBUS_UART_PORT, SOC_UART_FIFO_LEN + 4, SOC_UART_FIFO_LEN + 4, MODBUS_TX, MODBUS_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	myUart->Initialize();
	myUart->SetBaudRate(115200);
	myUart->SetDataBits(8);
	myUart->SetParity(PL::UartParity::even);
	myUart->SetStopBits(PL::UartStopBits::one);
	myUart->SetFlowControl(PL::UartFlowControl::none);
	myUart->Enable();

	myModbusServer = std::make_shared<PL::ModbusServer>(myUart, PL::ModbusProtocol::rtu, 1);

	myHoldingRegisters = std::make_shared<DynamicHoldingRegisters>(PL::ModbusMemoryType::holdingRegisters, 0, sizeof(DynamicHoldingRegisters));
	myCoils = std::make_shared<DynamicCoils>(PL::ModbusMemoryType::coils, 0, sizeof(DynamicHoldingRegisters));
	myDiscreteInputs = std::make_shared<DynamicDiscreteInputs>(PL::ModbusMemoryType::discreteInputs, 0, sizeof(DynamicHoldingRegisters));
	myInputRegisters = std::make_shared<DynamicInputRegisters>(PL::ModbusMemoryType::inputRegisters, 0, sizeof(DynamicHoldingRegisters));

	myModbusServer->AddMemoryArea(myCoils);
	myModbusServer->AddMemoryArea(myDiscreteInputs);
	myModbusServer->AddMemoryArea(myHoldingRegisters);
	myModbusServer->AddMemoryArea(myInputRegisters);
	myModbusServer->Enable();
}

esp_err_t DynamicDiscreteInputs::OnRead()
{
	State *state = State::GetInstance();
	data.ERROR = state->HasError();
	return ESP_OK;
}

esp_err_t DynamicInputRegisters::OnRead()
{
	State *state = State::GetInstance();
	TempController *controller = TempController::GetInstance();
	data.CURRENT_TEMP = controller->GetCurrentTemp();
	data.ERROR_CODE = static_cast<uint16_t>(state->GetError());
	data.HEATER_PWM_DUTY_CYCLE = controller->GetPwmDutyCycle();

	return ESP_OK;
}

esp_err_t DynamicHoldingRegisters::OnRead()
{
	State::GetInstance();
	TempController *controller = TempController::GetInstance();
	TempController::Config config = controller->GetConfig();

	data.HEATER_REDUCE_PWM_UNDER = config.SSR_REDUCED_PWM_UNDER;
	data.HEATER_REDUCED_PWM_VALUE = (config.SSR_REDUCED_PWM_VALUE / config.SSR_FULL_PWM) * 100;
	data.HEATER_PERIOD = config.PWM_PERIOD_MS;
	data.P = config.P;
	data.I = config.I;
	data.D = config.D;
	data.PID_WINDOW = config.SSR_BANG_BANG_WINDOW;
	data.TARGET_TEMP = controller->GetTargetTemp();

	return ESP_OK;
}

esp_err_t DynamicHoldingRegisters::OnWrite()
{
	TempController *controller = TempController::GetInstance();
	TempController::Config config = controller->GetConfig();
	config.SSR_REDUCED_PWM_UNDER = data.HEATER_REDUCE_PWM_UNDER;
	config.SSR_REDUCED_PWM_VALUE = data.HEATER_REDUCED_PWM_VALUE;
	config.PWM_PERIOD_MS = data.HEATER_PERIOD;
	config.P = data.P;
	config.I = data.I;
	config.D = data.D;
	config.SSR_BANG_BANG_WINDOW = data.PID_WINDOW;
	controller->SetConfig(config);
	controller->SetTargetTemp(data.TARGET_TEMP);

	return ESP_OK;
}

esp_err_t DynamicCoils::OnRead()
{
	State *state = State::GetInstance();
	data.ENABLE = state->IsEnabled();
	return ESP_OK;
}
esp_err_t DynamicCoils::OnWrite()
{
	State *state = State::GetInstance();
	if (data.ENABLE)
	{
		state->SetEnabled(true);
	}
	else
	{
		state->SetEnabled(false);
	}
	return ESP_OK;
}
#include "Client.hxx"
#include "Proto.hxx"
#include "uart/Uart.hxx"
#include <esp_log.h>
#include <pl_modbus.h>
#include <pl_uart.h>

const char *MODBUS_TAG = "ModbusClient";

Client *Client::myInstance = nullptr;

Client::Client()
{
	myUart = UARTManager::GetInstance()->GetUart();
	myClient = new PL::ModbusClient(myUart, PL::ModbusProtocol::rtu, 1);
	myInstance = this;
	myCoils = 0;
	myDiscreteInputs = 0;

	if (myClient->ReadHoldingRegisters(0, sizeof(myHeaterConfiguration) / sizeof(uint16_t), myHeaterConfiguration, NULL) == ESP_OK)
	{
		ESP_LOGI(MODBUS_TAG, "Holding registers read successfully");
	}
	else
	{
		ESP_LOGE(MODBUS_TAG, "Failed to read holding registers");
	}

	if (myClient->ReadCoils(0, 3, &myCoils, NULL) == ESP_OK)
	{
		ESP_LOGI(MODBUS_TAG, "Coils read successfully");
	}
	else
	{
		ESP_LOGE(MODBUS_TAG, "Failed to read coils");
	}

	if (myClient->ReadDiscreteInputs(0, 3, &myDiscreteInputs, NULL) == ESP_OK)
	{
		ESP_LOGI(MODBUS_TAG, "Discrete inputs read successfully");
	}
	else
	{
		ESP_LOGE(MODBUS_TAG, "Failed to read discrete inputs");
	}

	if (myClient->ReadInputRegisters(0, sizeof(myInputRegisters) / sizeof(uint16_t), myInputRegisters, NULL) == ESP_OK)
	{
		ESP_LOGI(MODBUS_TAG, "Input registers read successfully");
	}
	else
	{
		ESP_LOGE(MODBUS_TAG, "Failed to read input registers");
	}

	ESP_LOGI(MODBUS_TAG, "Modbus server initialized");
}

uint16_t Client::GetHoldingRegister(HoldingRegister reg)
{
	return myHeaterConfiguration[static_cast<int>(reg)];
}

uint16_t Client::GetInputRegister(InputRegister reg)
{
	return myInputRegisters[static_cast<int>(reg)];
}

uint16_t Client::GetCurrentPWMDutyCycle()
{
	return GetInputRegister(InputRegister::HEATER_PWM_DUTY_CYCLE);
}

uint16_t Client::GetCurrentTemp()
{
	return GetInputRegister(InputRegister::CURRENT_TEMP);
}

uint16_t Client::GetTargetTemp()
{
	return GetHoldingRegister(HoldingRegister::TARGET_TEMP);
}

uint16_t Client::GetErrorCode()
{
	return GetInputRegister(InputRegister::ERROR_CODE);
}

bool Client::GetMode()
{
	return IsDiscreteInputSet(DiscreteInputMask::MODE);
}

bool Client::HasError()
{
	return IsDiscreteInputSet(DiscreteInputMask::ERROR);
}

bool Client::IsDoorOpen()
{
	return IsDiscreteInputSet(DiscreteInputMask::DOOR_OPEN);
}

bool Client::IsEmergencyRelayActive()
{
	return IsDiscreteInputSet(DiscreteInputMask::EMERGENCY_RELAY);
}

Client *Client::GetInstance()
{
	if (myInstance == nullptr)
	{
		myInstance = new Client();
	}
	return myInstance;
}

bool Client::SetCoil(CoilMask mask, bool value)
{
	assert(myClient != nullptr);

	PL::ModbusException err;

	if (myClient->WriteSingleCoil(static_cast<uint16_t>(mask), value, &err) != ESP_OK)
	{
		return false;
	}

	return true;
}
bool Client::SetConfig(uint16_t p, uint16_t i, uint16_t d, uint16_t period, uint16_t pidWindow, uint16_t reducedPwmValue, uint16_t reducedPwmUnder)
{
	assert(myClient != nullptr);
	assert(myHeaterConfiguration != nullptr);

	myHeaterConfiguration[static_cast<int>(HoldingRegister::P)] = p;
	myHeaterConfiguration[static_cast<int>(HoldingRegister::I)] = i;
	myHeaterConfiguration[static_cast<int>(HoldingRegister::D)] = d;
	myHeaterConfiguration[static_cast<int>(HoldingRegister::HEATER_PERIOD)] = period;
	myHeaterConfiguration[static_cast<int>(HoldingRegister::PID_WINDOW)] = pidWindow;
	myHeaterConfiguration[static_cast<int>(HoldingRegister::TARGET_TEMP)] = 0; // Set to 0 to disable PID
	myHeaterConfiguration[static_cast<int>(HoldingRegister::HEATER_REDUCED_PWM_VALUE)] = reducedPwmValue;
	myHeaterConfiguration[static_cast<int>(HoldingRegister::HEATER_REDUCE_PWM_UNDER)] = reducedPwmUnder;

	return WriteHoldingRegisters();
}

bool Client::SetTargetTemp(uint16_t targetTemp)
{
	assert(myClient != nullptr);
	assert(myHeaterConfiguration != nullptr);

	myHeaterConfiguration[static_cast<int>(HoldingRegister::TARGET_TEMP)] = targetTemp;

	auto result = myClient->WriteSingleHoldingRegister(static_cast<uint16_t>(HoldingRegister::TARGET_TEMP), targetTemp, NULL);

	return (result == ESP_OK);
}

bool Client::EnableHeating()
{
	assert(myClient != nullptr);

	auto res = myClient->WriteSingleCoil(static_cast<uint16_t>(CoilMask::ENABLE), true, NULL);
	return (res == ESP_OK);
}

bool Client::DisableHeating()
{
	assert(myClient != nullptr);

	auto res = myClient->WriteSingleCoil(static_cast<uint16_t>(CoilMask::ENABLE), false, NULL);
	return (res == ESP_OK);
}

bool Client::IsHeatingEnabled()
{
	return (myCoils & static_cast<uint8_t>(CoilMask::ENABLE)) != 0;
}

bool Client::ReadCoils()
{
	assert(myClient != nullptr);
	bool readError = false;

	if (myClient->ReadCoils(0, 3, &myCoils, NULL) != ESP_OK)
	{
		readError = true;
	}

	return readError;
}

bool Client::ReadHoldingRegisters()
{
	assert(myClient != nullptr);
	assert(myHeaterConfiguration != nullptr);

	bool readError = false;

	if (myClient->ReadHoldingRegisters(0, sizeof(myHeaterConfiguration) / sizeof(uint16_t), myHeaterConfiguration, NULL) != ESP_OK)
	{
		readError = true;
	}

	return readError;
}

bool Client::ReadDiscreteInputs()
{
	assert(myClient != nullptr);
	bool readError = false;

	if (myClient->ReadDiscreteInputs(0, 3, &myDiscreteInputs, NULL) != ESP_OK)
	{
		readError = true;
	}

	return readError;
}

bool Client::ReadInputRegisters()
{
	assert(myClient != nullptr);
	assert(myInputRegisters != nullptr);

	bool readError = false;

	if (myClient->ReadInputRegisters(0, sizeof(myInputRegisters) / sizeof(uint16_t), myInputRegisters, NULL) != ESP_OK)
	{
		readError = true;
	}

	return readError;
}

bool Client::WriteHoldingRegisters()
{
	assert(myClient != nullptr);
	assert(myHeaterConfiguration != nullptr);

	bool writeError = false;

	if (myClient->WriteMultipleHoldingRegisters(0, sizeof(myHeaterConfiguration) / sizeof(uint16_t), myHeaterConfiguration, NULL) != ESP_OK)
	{
		writeError = true;
	}

	return writeError;
}

void Client::ReadTask(void *pvParameter)
{
	Client *instance = static_cast<Client *>(pvParameter);

	while (42)
	{
		if (instance == nullptr)
		{
			ESP_LOGE(MODBUS_TAG, "instance is null");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		if (instance->myClient == nullptr)
		{
			ESP_LOGE(MODBUS_TAG, "myClient is null");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}

		ReadDiscreteInputs();
		ReadInputRegisters();

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
#include <math.h>
#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <esp_check.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>

#include "GPIO.hxx"
#include "TempController.hxx"

#include "Console.hxx"
#include "SPIBus.hxx"
#include "Server.hxx"
#include "State.hxx"
#include "TempDevice.hxx"
#include "hardware.h"
#include "sdkconfig.h"
#include "uart/Uart.hxx"

#define TAG "MeltingFurnace"

extern "C" void app_main(void)
{

	GPIOManager::GetInstance();
	State::GetInstance();
	SPIBusManager *spi3Manager = new SPIBusManager(SPI3_HOST);
	// TempDevice *thermocouple = new MAX31856TempDevice(spi3Manager, MAX31856_SPI3_CS);
	//  thermocouple->SetType(TempType::TCTYPE_K);
	//  thermocouple->SetTempFaultThresholds(1350, 5);
	//  TempController controller(thermocouple, spi3Manager);
	TempDevice *simulatedThermocouple = new SimulatedTempDevice();
	simulatedThermocouple->SetType(TempType::TCTYPE_K);
	simulatedThermocouple->SetTempFaultThresholds(1350, 5);
	static_cast<SimulatedTempDevice *>(simulatedThermocouple)->SetTemp(25.0);
	// TempController controller(thermocouple, spi3Manager);
	TempController controller(simulatedThermocouple, spi3Manager);
	UARTManager::GetInstance();
	Server::GetInstance();
	Console::GetInstance();
	vTaskDelay(portMAX_DELAY);
}
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

#define TAG "MeltingFurnace"
#include "GPIO.hxx"
#include "TempController.hxx"

#include "SPIBus.hxx"
#include "Server.hxx"
#include "State.hxx"
#include "sdkconfig.h"
#include "uart/Uart.hxx"

extern "C" void app_main(void)
{

	GPIOManager::GetInstance();
	SPIBusManager *spi3Manager = new SPIBusManager(SPI3_HOST);
	TempController controller(spi3Manager);
	UARTManager::GetInstance();
	Server::GetInstance();
	vTaskDelay(portMAX_DELAY);
}
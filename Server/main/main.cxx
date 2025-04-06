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
#include "tempController.hxx"

#include "SPIBus.hxx"
#include "sdkconfig.h"

extern "C" void app_main(void)
{
	
	FurnaceClient::GetInstance();
	GPIOManager *gpio = new GPIOManager();
	SPIBusManager *spi3Manager = new SPIBusManager(SPI3_HOST);
	TempController controller(ui, spi3Manager);

	vTaskDelay(portMAX_DELAY);
}
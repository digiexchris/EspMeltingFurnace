#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

#include "max31856-espidf/max31856.hxx"

#include "tempController.hxx"

const char *TCTAG = "TempController";

TempController::TempController(TempUI *aTempUi, SPIBusManager *aBusManager) : myTempUi(aTempUi), mySpiBusManager(aBusManager), myInstance(nullptr), myThermocoupleQueue(nullptr)
{
	ESP_LOGI(TCTAG, "Initialize");

	myInstance = this;

	assert(aBusManager != nullptr);
	assert(aTempUi != nullptr);

	mySpiBusManager->lock();
	myThermocouple = new MAX31856::MAX31856(SPI3_HOST);
	myThermocouple->AddDevice(MAX31856::ThermocoupleType::MAX31856_TCTYPE_K, GPIO_NUM_27, 0);
	mySpiBusManager->unlock();

	myThermocoupleQueue = xQueueCreate(5, sizeof(struct MAX31856::Result));
	if (myThermocoupleQueue != NULL)
	{
		ESP_LOGI(TCTAG, "Created: thermocouple_queue");
	}
	else
	{
		ESP_LOGE(TCTAG, "Failed to Create: thermocouple_queue");
		assert(false);
	}

	xTaskCreate(&thermocoupleTask, "thermocouple_task", 2048, this, 5, NULL);
	xTaskCreate(&receiverTask, "receiver_task", 2048, this, 5, NULL);
}

void TempController::thermocoupleTask(void *pvParameter)
{
	ESP_LOGI("task", "Created: receiver_task");
	TempController *instance = static_cast<TempController *>(pvParameter);

	MAX31856::Result result = {
		.coldjunction_c = 0,
		.coldjunction_f = 0,
		.thermocouple_c = 0,
		.thermocouple_f = 0,
		.fault = 0,
	};

	while (42)
	{
		instance->mySpiBusManager->lock();
		instance->myThermocouple->read(result, 0);
		instance->mySpiBusManager->unlock();
		xQueueSend(instance->myThermocoupleQueue, &result, (TickType_t)0);
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
}

void TempController::receiverTask(void *pvParameter)
{
	// TODO this probably belongs to main instead. Maybe call a callback from here passing the result back?
	ESP_LOGI("task", "Created: thermocouple_task");
	TempController *instance = static_cast<TempController *>(pvParameter);

	MAX31856::Result result = {
		.coldjunction_c = 0,
		.coldjunction_f = 0,
		.thermocouple_c = 0,
		.thermocouple_f = 0,
		.fault = 0,
	};

	while (1)
	{
		xQueueReceive(instance->myThermocoupleQueue, &result, (TickType_t)(200 / portTICK_PERIOD_MS));
		printf("Struct Received on Queue:\nCold Junction: %0.2f\nTemperature: %0.2f\n", result.coldjunction_f, result.thermocouple_f);
		// printf("Fault: %d\n", max31856_result.fault);
		printf("Cold Junction: %0.2f\n", result.coldjunction_c);
		printf("Thermocouple: %0.2f\n", result.thermocouple_c);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

#include "TempDevice.hxx"

MAX31856TempDevice::MAX31856TempDevice(SPIBusManager *aBusManager, gpio_num_t csPin) : myCsPin(csPin), myBusManager(aBusManager)
{
	assert(aBusManager != nullptr);

	mySpiBusManager->lock();
	myThermocouple = new MAX31856::MAX31856(SPI3_HOST, false);
	myThermocouple->AddDevice(MAX31856::ThermocoupleType::MAX31856_TCTYPE_K, MAX31856_SPI3_CS, 0);
	mySpiBusManager->unlock();

	myThermocoupleQueue = xQueueCreate(5, sizeof(struct TempResult));
	if (myThermocoupleQueue != NULL)
	{
		ESP_LOGI(TCTAG, "Created: thermocouple_queue");
	}
	else
	{
		ESP_LOGE(TCTAG, "Failed to Create: thermocouple_queue");
		assert(false);
	}

	xTaskCreate(&thermocoupleTask, "thermocouple_task", 2048, this, 6, NULL);
	xTaskCreate(&receiverTask, "receiver_task", 2048, this, 6, NULL);
}

void MAX31856TempDevice::thermocoupleTask(void *pvParameter)
{
	ESP_LOGI("task", "Created: receiver_task");
	MAX31856TempDevice *instance = static_cast<MAX31856TempDevice *>(pvParameter);

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
		myLastResult = result;
		vTaskDelay(600 / portTICK_PERIOD_MS);
	}
}

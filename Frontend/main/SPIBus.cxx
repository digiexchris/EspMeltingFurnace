#include "SPIBus.hxx"
#include <esp_log.h>

const char *TAG = "SPIBusManager";

SPIBusManager::SPIBusManager(spi_host_device_t aHost, spi_bus_config_t aBusCfg, spi_common_dma_t aDmaChannel, int maxTransferSize) : myHost(aHost)
{

	ESP_ERROR_CHECK(spi_bus_initialize(myHost, &aBusCfg, aDmaChannel));

	myMutex = xSemaphoreCreateMutex();
	if (myMutex == NULL)
	{
		ESP_LOGE(TAG, "Failed to create mutex");
		abort();
	}
}

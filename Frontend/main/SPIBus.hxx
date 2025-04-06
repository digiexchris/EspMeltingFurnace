#pragma once

#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// spi_bus_config_t defaultSpi3BusConfig = {
// 	.mosi_io_num = GPIO_NUM_32,
// 	.miso_io_num = GPIO_NUM_39,
// 	.sclk_io_num = GPIO_NUM_25,
// 	.quadwp_io_num = -1,
// 	.quadhd_io_num = -1,
// 	.max_transfer_sz = 0,
// };

class SPIBusManager
{
public:
	static constexpr spi_bus_config_t defaultSpi3BusConfig = {.mosi_io_num = GPIO_NUM_32,
															  .miso_io_num = GPIO_NUM_39,
															  .sclk_io_num = GPIO_NUM_25,
															  .quadwp_io_num = GPIO_NUM_NC,
															  .quadhd_io_num = GPIO_NUM_NC,
															  .data4_io_num = GPIO_NUM_NC,
															  .data5_io_num = GPIO_NUM_NC,
															  .data6_io_num = GPIO_NUM_NC,
															  .data7_io_num = GPIO_NUM_NC,
															  .max_transfer_sz = 32768,
															  .flags = SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO | SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
															  .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
															  .intr_flags = ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM};

	SPIBusManager(spi_host_device_t host = SPI3_HOST, spi_bus_config_t aBusConfig = defaultSpi3BusConfig, spi_common_dma_t aDmaChannel = SPI_DMA_DISABLED, int maxTransferSize = 0);

	esp_err_t lock(TickType_t waitTime = portMAX_DELAY)
	{
		return xSemaphoreTake(myMutex, waitTime);
	}

	esp_err_t unlock()
	{
		return xSemaphoreGive(myMutex);
	}

	SemaphoreHandle_t getMutex()
	{
		return myMutex;
	}

	spi_host_device_t getHost()
	{
		return myHost;
	}

private:
	SemaphoreHandle_t myMutex;

	spi_host_device_t myHost;
};
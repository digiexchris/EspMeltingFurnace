/**
 * @file spi_port.c
 * @author by mondraker (https://oshwhub.com/mondraker)(https://github.com/HwzLoveDz)
 * @brief spi port,easy component migration
 * @version 0.1
 * @date 2023-07-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "spi_port.h"
#include "freertos/semphr.h"
#include "esp_rom_sys.h" // For esp_rom_delay_us

// Semaphore for SPI3 bus access synchronization
SemaphoreHandle_t spi3_bus_mutex = NULL;

/**
 * @brief Initialize the SPI3 bus mutex
 */
esp_err_t spi3_bus_mutex_init(void)
{
	if (spi3_bus_mutex == NULL)
	{
		spi3_bus_mutex = xSemaphoreCreateMutex();
		if (spi3_bus_mutex == NULL)
		{
			ESP_LOGE("SPI", "Failed to create SPI3 bus mutex");
			return ESP_FAIL;
		}
	}
	return ESP_OK;
}

/**
 * @brief Take the SPI3 bus mutex
 * @param timeout_ms Timeout in milliseconds
 * @return true if successful, false if timed out
 */
bool spi3_bus_take(uint32_t timeout_ms)
{
	if (spi3_bus_mutex == NULL)
	{
		ESP_LOGE("SPI", "SPI3 bus mutex not initialized");
		return false;
	}

	if (xSemaphoreTake(spi3_bus_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE)
	{
		return true;
	}
	else
	{
		ESP_LOGW("SPI", "Failed to take SPI3 bus mutex (timeout: %lu ms)", timeout_ms);
		return false;
	}
}

/**
 * @brief Release the SPI3 bus mutex
 */
void spi3_bus_give(void)
{
	if (spi3_bus_mutex != NULL)
	{
		xSemaphoreGive(spi3_bus_mutex);
	}
}

/**
 * @brief spi master initialization
 */
esp_err_t spi_master_init()
{
	esp_err_t ret;

	// Initialize the SPI3 bus mutex
	ret = spi3_bus_mutex_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE("SPI", "Failed to initialize SPI3 mutex");
		return ret;
	}

	// We don't need to initialize the SPI bus again since it's already done in touch.c
	// We only need to add a device to the existing bus
	spi_device_interface_config_t devcfg = {
		.mode = 1,							  // SPI mode 1 (CPOL=0, CPHA=1) for MAX31856
		.clock_speed_hz = SPI_MASTER_FREQ_8M, // Slower clock for reliability (1 MHz)
		/*
		 * The timing requirements to read the busy signal from the EEPROM cannot be easily emulated
		 * by SPI transactions. We need to control CS pin by SW to check the busy signal manually.
		 */
		.spics_io_num = -1, // CS pin managed by software
		.queue_size = 7,	// Queue size
	};

	// Acquire the SPI3 bus mutex before adding our device
	if (!spi3_bus_take(SPI3_BUS_TIMEOUT_MS))
	{
		ESP_LOGE("SPI", "Failed to acquire SPI3 bus during initialization");
		return ESP_ERR_TIMEOUT;
	}

	// Add our device to the existing bus
	ret = spi_bus_add_device(TOUCH_SPI, &devcfg, &spi_dev);
	if (ret != ESP_OK)
	{
		spi3_bus_give(); // Release the mutex on error
		ESP_LOGE("SPI", "Failed to add MAX31856 to SPI3 bus: %s", esp_err_to_name(ret));
		return ret;
	}

	// Configure CS pin
	gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);
	gpio_set_level(PIN_NUM_CS, 1); // Default high (inactive)

	// Release the SPI3 bus mutex
	spi3_bus_give();

	return ESP_OK;
}

uint32_t spi_write(spi_device_handle_t spi, uint8_t *data, uint8_t len)
{
	esp_err_t ret;
	spi_transaction_t t;

	// Take the SPI3 bus mutex with timeout
	if (!spi3_bus_take(SPI3_BUS_TIMEOUT_MS))
	{
		ESP_LOGE("SPI", "Failed to acquire SPI3 bus for write operation");
		return ESP_ERR_TIMEOUT;
	}

	// Prepare transaction
	memset(&t, 0, sizeof(t)); // Zero out the transaction
	gpio_set_level(PIN_NUM_CS, 0);
	// CS setup time - brief delay
	esp_rom_delay_us(10);
	t.length = len * 8; // Len is in bytes, transaction length is in bits.
	t.tx_buffer = data; // Data
	t.user = (void *)1; // D/C needs to be set to 1

	// Queue transaction with timeout
	ret = spi_device_queue_trans(spi, &t, 0);
	if (ret != ESP_OK)
	{
		ESP_LOGE("SPI", "Failed to queue SPI write transaction: %s", esp_err_to_name(ret));
		gpio_set_level(PIN_NUM_CS, 1); // Release CS on error
		spi3_bus_give();			   // Release the mutex
		return ret;
	}

	// Wait for transaction to complete with timeout
	spi_transaction_t *rtrans;
	ret = spi_device_get_trans_result(spi, &rtrans, 500 / portTICK_PERIOD_MS);
	if (ret != ESP_OK)
	{
		ESP_LOGE("SPI", "SPI write transaction timed out: %s", esp_err_to_name(ret));
	}

	// Brief delay before deasserting CS
	esp_rom_delay_us(10);
	gpio_set_level(PIN_NUM_CS, 1);
	// CS hold time
	esp_rom_delay_us(10);

	// Release the SPI3 bus mutex
	spi3_bus_give();

	return ret;
}

uint32_t spi_read(spi_device_handle_t spi, uint8_t *data, uint8_t len)
{
	esp_err_t ret;
	spi_transaction_t t;

	// Take the SPI3 bus mutex with timeout
	if (!spi3_bus_take(SPI3_BUS_TIMEOUT_MS))
	{
		ESP_LOGE("SPI", "Failed to acquire SPI3 bus for read operation");
		return ESP_ERR_TIMEOUT;
	}

	// Prepare transaction
	memset(&t, 0, sizeof(t));
	gpio_set_level(PIN_NUM_CS, 0);
	// CS setup time - brief delay
	esp_rom_delay_us(10);
	t.length = len * 8;
	t.rx_buffer = data; // Use rx_buffer for read operations
	t.user = (void *)1;

	// Queue transaction with timeout
	ret = spi_device_queue_trans(spi, &t, 0);
	if (ret != ESP_OK)
	{
		ESP_LOGE("SPI", "Failed to queue SPI read transaction: %s", esp_err_to_name(ret));
		gpio_set_level(PIN_NUM_CS, 1); // Release CS on error
		spi3_bus_give();			   // Release the mutex
		return ret;
	}

	// Wait for transaction to complete with timeout
	spi_transaction_t *rtrans;
	ret = spi_device_get_trans_result(spi, &rtrans, 500 / portTICK_PERIOD_MS);
	if (ret != ESP_OK)
	{
		ESP_LOGE("SPI", "SPI read transaction timed out: %s", esp_err_to_name(ret));
	}

	// Brief delay before deasserting CS
	esp_rom_delay_us(10);
	gpio_set_level(PIN_NUM_CS, 1);
	// CS hold time
	esp_rom_delay_us(10);

	// Release the SPI3 bus mutex
	spi3_bus_give();

	return ret;
}

spi_device_handle_t spi_dev;

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

/**
 * @brief spi master initialization
 */
esp_err_t spi_master_init()
{
	esp_err_t ret;

	// We don't need to initialize the SPI bus again since it's already done in touch.c
	// We only need to add a device to the existing bus
	spi_device_interface_config_t devcfg = {
		.mode = 0,							   // SPI mode 0
		.clock_speed_hz = SPI_MASTER_FREQ_10M, // Clock out at 10 MHz
		/*
		 * The timing requirements to read the busy signal from the EEPROM cannot be easily emulated
		 * by SPI transactions. We need to control CS pin by SW to check the busy signal manually.
		 */
		.spics_io_num = -1, // CS pin managed by software
		.queue_size = 7,	// Queue size
	};

	// Don't initialize the SPI bus, just add our device to the existing bus
	ret = spi_bus_add_device(TOUCH_SPI, &devcfg, &spi_dev);
	if (ret != ESP_OK)
	{
		return ret;
	}

	// Configure CS pin
	gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);
	gpio_set_level(PIN_NUM_CS, 1); // Default high (inactive)

	return ESP_OK;
}

uint32_t spi_write(spi_device_handle_t spi, uint8_t *data, uint8_t len)
{
	esp_err_t ret;
	spi_transaction_t t;

	memset(&t, 0, sizeof(t)); // Zero out the transaction
	gpio_set_level(PIN_NUM_CS, 0);
	t.length = len * 8;							// Len is in bytes, transaction length is in bits.
	t.tx_buffer = data;							// Data
	t.user = (void *)1;							// D/C needs to be set to 1
	ret = spi_device_polling_transmit(spi, &t); // Transmit!
	ESP_ERROR_CHECK(ret);
	assert(ret == ESP_OK); // Should have had no issues.
	gpio_set_level(PIN_NUM_CS, 1);
	return ret;
}

uint32_t spi_read(spi_device_handle_t spi, uint8_t *data, uint8_t len)
{
	spi_transaction_t t;
	gpio_set_level(PIN_NUM_CS, 0);
	memset(&t, 0, sizeof(t));
	t.length = len * 8;
	t.flags = SPI_TRANS_USE_RXDATA;
	t.user = (void *)1;
	esp_err_t ret = spi_device_polling_transmit(spi, &t);
	assert(ret == ESP_OK);
	*data = t.rx_data[0];
	gpio_set_level(PIN_NUM_CS, 1);
	return ret;
}

spi_device_handle_t spi_dev;

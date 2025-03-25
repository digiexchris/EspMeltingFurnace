/**
 * @file spi_port.h
 * @author by mondraker (https://oshwhub.com/mondraker)(https://github.com/HwzLoveDz)
 * @brief
 * @version 0.1
 * @date 2023-07-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "sdkconfig.h"

// Use the same SPI host as defined for touch
// #define SPI_HOST TOUCH_SPI
#define PIN_NUM_CS GPIO_NUM_27

	esp_err_t spi_master_init();
	uint32_t spi_write(spi_device_handle_t spi, uint8_t *data, uint8_t len);
	uint32_t spi_read(spi_device_handle_t spi, uint8_t *data, uint8_t len);

	extern spi_device_handle_t spi_dev;

#ifdef __cplusplus
}
#endif

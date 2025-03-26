#pragma once

#include <esp_err.h>
#include <esp_lcd_touch.h>

#ifdef __cplusplus
extern "C"
{
#endif

	// Will not init the bus, use SPIBusManager for that
	esp_err_t touch_init(spi_host_device_t host, SemaphoreHandle_t aMutex, esp_lcd_touch_handle_t *tp);

#ifdef __cplusplus
}
#endif
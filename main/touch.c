// #include <freertos/FreeRTOS.h>
// #include <freertos/semphr.h>
// #include <freertos/task.h>

// #include <esp_err.h>
// #include <esp_lcd_touch.h>
// #include <esp_lcd_touch_xpt2046.h>
// #include <esp_log.h>
// #include <esp_timer.h>

// #include <driver/gpio.h>
// #include <driver/spi_master.h>

// #include "hardware.h"
// #include "lvgl.h"
// #include "lvgl_private.h"
// #include "touch.h"

// esp_lcd_touch_handle_t tp = NULL;

// static uint16_t
// map(uint16_t n, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
// {
// 	uint16_t value = (n - in_min) * (out_max - out_min) / (in_max - in_min);
// 	return (value < out_min) ? out_min : ((value > out_max) ? out_max : value);
// }

// static void process_coordinates(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
// {
// 	ESP_LOGI("Touch", "Processing coordinates");
// 	*x = map(*x, TOUCH_X_RES_MIN, TOUCH_X_RES_MAX, 0, LCD_H_RES);
// 	*y = map(*y, TOUCH_Y_RES_MIN, TOUCH_Y_RES_MAX, 0, LCD_V_RES);
// }

// static void touch_interrupt_callback(esp_lcd_touch_handle_t tp, void *arg)
// {
// 	ESP_LOGI("Touch", "Interrupt callback");
// 	esp_lcd_touch_read_data(tp);
// }

// void touch_driver_read(lv_indev_t *drv, lv_indev_data_t *data)
// {
// 	uint16_t x[1];
// 	uint16_t y[1];
// 	uint16_t strength[1];
// 	uint8_t count = 0;

// 	// Update touch point data.
// 	ESP_ERROR_CHECK(esp_lcd_touch_read_data(tp));

// 	data->state = LV_INDEV_STATE_REL;

// 	if (esp_lcd_touch_get_coordinates(tp, x, y, strength, &count, 1))
// 	{
// 		data->point.x = x[0];
// 		data->point.y = y[0];
// 		data->state = LV_INDEV_STATE_PR;
// 	}

// 	data->continue_reading = false;
// }
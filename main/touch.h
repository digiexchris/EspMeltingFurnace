#pragma once

#include <esp_err.h>
#include <esp_lcd_touch.h>

#ifdef __cplusplus
extern "C"
{
#endif

	esp_err_t touch_init(esp_lcd_touch_handle_t *tp);

#ifdef __cplusplus
}
#endif
#include <math.h>
#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <esp_check.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>

#include <esp_lvgl_port.h>
#include <lvgl.h>

extern "C"
{
#include "lcd.h"
#include "touch.h"
}

#include "ui/ui.h"

#define TAG "MeltingFurnace"

// Global variables
bool started = false;
float currentTemp = 10.0;
float setTemp = 10.0;
float lowerLimit = 10.0;
float upperLimit = 1350.0;

void updateNumberLabel(lv_obj_t *label, float value)
{
	char text_buffer[32];
	sprintf(text_buffer, "%0.1f", value);

	lvgl_port_lock(0);
	lv_label_set_text(label, text_buffer);
	lvgl_port_unlock();
}

extern void updateCurrentTemp(float value)
{
	currentTemp = value;
	updateNumberLabel(ui_CurrentTemp, currentTemp);
}

extern void updateSetTemp(float value)
{
	setTemp = value;
	updateNumberLabel(ui_SetTemp, setTemp);
}

extern void updateLowerLimit(float value)
{
	lowerLimit = value;
	updateNumberLabel(ui_LowerLimit, lowerLimit);
}

extern void updateUpperLimit(float value)
{
	upperLimit = value;
	updateNumberLabel(ui_UpperLimit, upperLimit);
}

void ui_event_Screen(lv_event_t *e)
{
	lv_event_code_t event_code = lv_event_get_code(e);

	if (event_code == LV_EVENT_CLICKED)
	{
		// lv_obj_align(btn, pos++, 0, 0);
		// if (pos > 9)
		// 	pos = 1;
	}
}

void OnToggleStartStop(lv_event_t *e)
{
	lvgl_port_lock(0);
	if (started)
	{
		started = false;
		lv_label_set_text(ui_OnOffButtonLabel, "Start");
	}
	else
	{
		started = true;
		lv_label_set_text(ui_OnOffButtonLabel, "Stop");
	}
	lvgl_port_unlock();
}

void OnSetTempChanged(lv_event_t *e)
{
	lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
	lv_event_code_t event_code = lv_event_get_code(e);

	if (event_code == LV_EVENT_VALUE_CHANGED)
	{
		setTemp = lv_arc_get_value(target);
		updateSetTemp(setTemp);
	}
}

static esp_err_t app_lvgl_main(void)
{
	// lv_obj_t *scr = lv_scr_act();

	lvgl_port_lock(0);

	ui_init();

	__attribute__((unused)) auto disp = lv_disp_get_default();
	// lv_disp_set_rotation(disp, LV_DISP_ROTATION_90);
	//   lv_disp_set_rotation(disp, LV_DISP_ROT_180);
	//   lv_disp_set_rotation(disp, LV_DISP_ROT_270);

	updateCurrentTemp(currentTemp);
	updateSetTemp(setTemp);
	updateLowerLimit(lowerLimit);
	updateUpperLimit(upperLimit);

	lvgl_port_unlock();

	return ESP_OK;
}
extern "C" void app_main(void)
{
	esp_lcd_panel_io_handle_t lcd_io;
	esp_lcd_panel_handle_t lcd_panel;
	esp_lcd_touch_handle_t tp;
	lvgl_port_touch_cfg_t touch_cfg;
	lv_display_t *lvgl_display = NULL;

	ESP_ERROR_CHECK(lcd_display_brightness_init());

	ESP_ERROR_CHECK(app_lcd_init(&lcd_io, &lcd_panel));
	lvgl_display = app_lvgl_init(lcd_io, lcd_panel);
	if (lvgl_display == NULL)
	{
		ESP_LOGI(TAG, "fatal error in app_lvgl_init");
		esp_restart();
	}

	ESP_ERROR_CHECK(touch_init(&tp));
	touch_cfg.disp = lvgl_display;
	touch_cfg.handle = tp;
	lvgl_port_add_touch(&touch_cfg);

	ESP_ERROR_CHECK(lcd_display_brightness_set(75));
	// ESP_ERROR_CHECK(lcd_display_rotate(lvgl_display, LV_DISPLAY_ROTATION_90));
	ESP_ERROR_CHECK(app_lvgl_main());

	while (42)
	{
		// sprintf(buf, "%04d", n++);

		if (lvgl_port_lock(0))
		{
			// update some ui

			lvgl_port_unlock();
		}

		vTaskDelay(125 / portTICK_PERIOD_MS);
	}
	vTaskDelay(portMAX_DELAY);
}
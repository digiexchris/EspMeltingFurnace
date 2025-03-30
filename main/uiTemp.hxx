#pragma once

#include "SPIBus.hxx"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h" // For PCNT peripheral
#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd.h"
#include "lvgl.h"
// #include "touch.h"
#include "hardware.h"

// PCNT configurations
#define PCNT_HIGH_LIMIT 100
#define PCNT_LOW_LIMIT -100

extern TaskHandle_t updateTaskHandle;

class TempUI
{
public:
	using Callback = void (*)(lv_event_t *);

	TempUI(SPIBusManager *aBusManager);
	~TempUI();

	// Getter methods for UI elements that might need to be accessed externally
	lv_obj_t *GetArc() { return ui_Arc1; }
	lv_obj_t *GetCurrentTempLabel() { return ui_CurrentTemp; }
	lv_obj_t *GetSetTempLabel() { return ui_SetTemp; }
	lv_obj_t *GetOnOffButton() { return ui_OnOff; }

	void SetError(bool error)
	{
		lvgl_port_lock(0);
		if (error)
		{
			Stop();
			lv_obj_set_style_bg_color(ui_Temp, lv_color_hex(0xFF0000), LV_PART_MAIN);
			lv_label_set_text(ui_OnOffButtonLabel, "Error");
			errored = true;
		}
		else
		{
			lv_obj_set_style_bg_color(ui_Temp, lv_color_hex(0x000000), LV_PART_MAIN);
			lv_label_set_text(ui_OnOffButtonLabel, "Start");
			errored = false;
		}
		lvgl_port_unlock();
	}

	bool IsError()
	{
		return errored;
	}

	void SetCurrentTemp(int temp)
	{
		currentTemp = temp;
		UpdateTempLabel(ui_CurrentTemp, currentTemp);
	}

	void SetTargetTemp(int value)
	{
		lvgl_port_lock(0);
		setTemp = value;
		UpdateTempLabel(ui_SetTemp, setTemp);
		lvgl_port_unlock();
	}

	int GetTargetTemp() { return setTemp; }
	bool IsStarted() { return started; }
	void Stop();
	void Start();
	static TempUI *GetInstance() { return instance; }

private:
	static void UpdateTouchTask(void *param);

	static void ProcessTouchCoordinates(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
	{
		ESP_LOGI("Touch", "Processing coordinates");
		*x = map(*x, TOUCH_X_RES_MIN, TOUCH_X_RES_MAX, 0, LCD_H_RES);
		*y = map(*y, TOUCH_Y_RES_MIN, TOUCH_Y_RES_MAX, 0, LCD_V_RES);
	}

	static void TouchInterruptCallback(esp_lcd_touch_handle_t th)
	{
		// ESP_LOGI("Touch", "Interrupt callback");
		if (updateTaskHandle)
		{
			xTaskNotifyFromISR(updateTaskHandle, 0, eNoAction, NULL);
		}
	}

	static void TouchDriverRead(lv_indev_t *drv, lv_indev_data_t *data)
	{
		uint16_t x[1];
		uint16_t y[1];
		uint16_t strength[1];
		uint8_t count = 0;

		// Update touch point data.
		ESP_ERROR_CHECK(esp_lcd_touch_read_data(tp));

		data->state = LV_INDEV_STATE_REL;

		if (esp_lcd_touch_get_coordinates(tp, x, y, strength, &count, 1))
		{
			data->point.x = x[0];
			data->point.y = y[0];
			data->state = LV_INDEV_STATE_PR;
		}

		data->continue_reading = false;
	}

	bool errored = false;
	SPIBusManager *myBusManager;
	esp_lcd_panel_io_handle_t lcd_io;
	esp_lcd_panel_handle_t lcd_panel;
	lvgl_port_touch_cfg_t touch_cfg;
	static esp_lcd_touch_handle_t tp;

	lv_display_t *lvgl_display;
	lv_obj_t *ui_Temp;

	lv_obj_t *ui_Arc1;
	lv_obj_t *ui_CurrentTemp;
	lv_obj_t *ui_SetTemp;
	lv_obj_t *ui_LowerLimit;
	lv_obj_t *ui_UpperLimit;
	lv_obj_t *ui_OnOff;
	lv_obj_t *ui_OnOffButtonLabel;

	static constexpr int onOffStoppedColor = 0x00BC00;
	static constexpr int onOffRunningColor = 0xBC0000;

	bool started = false;
	int currentTemp = 10;
	int setTemp = 150;

	static TempUI *instance;

	// Event callback methods
	static void ui_event_Arc1(lv_event_t *e);
	static void ui_event_OnOff(lv_event_t *e);

	int lowerLimit = 10.0;
	int upperLimit = 1350.0;

	void UpdateTempLabel(lv_obj_t *label, int value)
	{
		char text_buffer[32];
		sprintf(text_buffer, "%dC", value);

		lvgl_port_lock(0);
		lv_label_set_text(label, text_buffer);
		lvgl_port_unlock();
	}

	void UpdateLowerLimit(int value)
	{
		lowerLimit = value;
		UpdateTempLabel(ui_LowerLimit, lowerLimit);
	}

	void UpdateUpperLimit(int value)
	{
		upperLimit = value;
		UpdateTempLabel(ui_UpperLimit, upperLimit);
	}

	static uint16_t
	map(uint16_t n, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
	{
		uint16_t value = (n - in_min) * (out_max - out_min) / (in_max - in_min);
		return (value < out_min) ? out_min : ((value > out_max) ? out_max : value);
	}

	// Rotary encoder variables
	pcnt_unit_handle_t pcnt_unit;
	pcnt_channel_handle_t pcnt_chan_a;
	pcnt_channel_handle_t pcnt_chan_b;

	// Last encoder position
	int16_t last_encoder_value = 0;

	// Temp change amount per step (can be adjusted)
	static constexpr int TEMP_CHANGE_AMOUNT = 10; // 10°C per step
};
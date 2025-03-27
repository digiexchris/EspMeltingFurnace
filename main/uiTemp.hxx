#pragma once

#include "lvgl.h"

#include "lcd.h"
#include "touch.h"

#include "SPIBus.hxx"

class TempUI
{
public:
	using Callback = void (*)(lv_event_t *);

	TempUI(SPIBusManager *aBusManager, Callback onSetTempChanged = nullptr, Callback onToggleStartStop = nullptr);
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

	int &GetTargetTemp() { return setTemp; }
	bool IsStarted() { return started; }
	void Stop() { started = false; }
	void Start() { started = true; }
	static TempUI *GetInstance() { return instance; }
	static esp_lcd_touch_handle_t tp;

private:
	bool errored = false;
	SPIBusManager *myBusManager;
	esp_lcd_panel_io_handle_t lcd_io;
	esp_lcd_panel_handle_t lcd_panel;

	lvgl_port_touch_cfg_t touch_cfg;
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

	// Callback functions for setting temperature and toggling start/stop
	Callback onSetTempChanged;
	Callback onToggleStartStop;
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
};
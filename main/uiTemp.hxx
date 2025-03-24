#pragma once

#include "lvgl.h"

#include "lcd.h"
#include "touch.h"

class TempUI
{
public:
	using Callback = void (*)(lv_event_t *);
	TempUI(Callback onSetTempChanged = nullptr, Callback onToggleStartStop = nullptr);
	~TempUI();

	// Getter methods for UI elements that might need to be accessed externally
	lv_obj_t *GetArc() { return ui_Arc1; }
	lv_obj_t *GetCurrentTempLabel() { return ui_CurrentTemp; }
	lv_obj_t *GetSetTempLabel() { return ui_SetTemp; }
	lv_obj_t *GetOnOffButton() { return ui_OnOff; }

	void SetCurrentTemp(int temp) { currentTemp = temp; }
	int GetSetTemp() { return setTemp; }
	bool IsStarted() { return started; }
	void Stop() { started = false; }
	void Start() { started = true; }
	static TempUI *GetInstance() { return instance; }

private:
	esp_lcd_panel_io_handle_t lcd_io;
	esp_lcd_panel_handle_t lcd_panel;
	esp_lcd_touch_handle_t tp;
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

	bool started = false;
	int currentTemp = 10;
	int setTemp = 10;

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

	void UpdateCurrentTemp(int value)
	{
		currentTemp = value;
		UpdateTempLabel(ui_CurrentTemp, currentTemp);
	}

	void UpdateSetTemp(int value)
	{
		setTemp = value;
		UpdateTempLabel(ui_SetTemp, setTemp);
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
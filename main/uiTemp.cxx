#include "uiTemp.hxx"
#include "SPIBus.hxx"
#include "esp_check.h"
#include "esp_log.h"
#include "lcd.h"
#include "touch.h"
#include <stdio.h>

static const char *TAG = "TempUI";

// Define the static instance variable
TempUI *TempUI::instance = nullptr;

TempUI::TempUI(SPIBusManager *aBusManager, Callback onSetTempChanged, Callback onToggleStartStop)
	: myBusManager(aBusManager), lcd_io(nullptr), lcd_panel(nullptr), tp(nullptr), lvgl_display(nullptr), ui_Temp(nullptr), ui_Arc1(nullptr), ui_CurrentTemp(nullptr), ui_SetTemp(nullptr), ui_LowerLimit(nullptr), ui_UpperLimit(nullptr), ui_OnOff(nullptr), ui_OnOffButtonLabel(nullptr),
	  onSetTempChanged(onSetTempChanged), onToggleStartStop(onToggleStartStop)
{
	instance = this;
	ESP_ERROR_CHECK(lcd_display_brightness_init());

	ESP_ERROR_CHECK(app_lcd_init(&lcd_io, &lcd_panel));
	lvgl_display = app_lvgl_init(lcd_io, lcd_panel);
	if (lvgl_display == NULL)
	{
		ESP_LOGI(TAG, "fatal error in app_lvgl_init");
		esp_restart();
	}

	// Initialize touch before UI setup
	ESP_ERROR_CHECK(touch_init(SPI3_HOST, myBusManager->getMutex(), &tp));
	touch_cfg.disp = lvgl_display;
	touch_cfg.handle = tp;
	lvgl_port_add_touch(&touch_cfg);

	lv_theme_t *theme = lv_theme_simple_init(lvgl_display);
	lv_disp_set_theme(lvgl_display, theme);

	printf("ui_Temp_screen_init\n");
	ui_Temp = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_Temp, LV_OBJ_FLAG_SCROLLABLE); /// Flags

	// Assuming ui_img_pattern_png might be defined elsewhere or needs to be created
	// Use static pattern or solid color if image isn't available
	lv_obj_set_style_bg_color(ui_Temp, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(ui_Temp, 255, LV_PART_MAIN);

	ui_Arc1 = lv_arc_create(ui_Temp);
	lv_obj_set_width(ui_Arc1, lv_pct(80));
	lv_obj_set_height(ui_Arc1, lv_pct(60));
	lv_obj_set_x(ui_Arc1, -2);
	lv_obj_set_y(ui_Arc1, -41);
	lv_obj_set_align(ui_Arc1, LV_ALIGN_CENTER);

	lv_obj_remove_flag(ui_Arc1, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE |
												LV_OBJ_FLAG_SCROLL_ELASTIC |
												LV_OBJ_FLAG_SCROLL_MOMENTUM |
												LV_OBJ_FLAG_SCROLL_CHAIN));

	lv_obj_set_scrollbar_mode(ui_Arc1, LV_SCROLLBAR_MODE_OFF);
	lv_arc_set_range(ui_Arc1, 0, 1350);
	lv_arc_set_value(ui_Arc1, 50);
	lv_obj_set_style_arc_color(ui_Arc1, lv_color_hex(0x7A7AE6), LV_PART_MAIN);
	lv_obj_set_style_arc_opa(ui_Arc1, 255, LV_PART_MAIN);
	lv_obj_set_style_arc_width(ui_Arc1, 10, LV_PART_MAIN);

	lv_obj_set_style_arc_color(ui_Arc1, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
	lv_obj_set_style_arc_opa(ui_Arc1, 255, LV_PART_INDICATOR);

	ui_CurrentTemp = lv_label_create(ui_Temp);
	lv_obj_set_width(ui_CurrentTemp, LV_SIZE_CONTENT);	/// 1
	lv_obj_set_height(ui_CurrentTemp, LV_SIZE_CONTENT); /// 1
	lv_obj_set_x(ui_CurrentTemp, -2);
	lv_obj_set_y(ui_CurrentTemp, -66);
	lv_obj_set_align(ui_CurrentTemp, LV_ALIGN_CENTER);
	lv_label_set_text(ui_CurrentTemp, "9999C");
	lv_obj_set_style_text_font(ui_CurrentTemp, &lv_font_montserrat_36, LV_PART_MAIN);

	ui_SetTemp = lv_label_create(ui_Temp);
	lv_obj_set_width(ui_SetTemp, LV_SIZE_CONTENT);	/// 1
	lv_obj_set_height(ui_SetTemp, LV_SIZE_CONTENT); /// 1
	lv_obj_set_x(ui_SetTemp, -2);
	lv_obj_set_y(ui_SetTemp, -16);
	lv_obj_set_align(ui_SetTemp, LV_ALIGN_CENTER);
	lv_label_set_text(ui_SetTemp, "1150C");
	lv_obj_set_style_text_font(ui_SetTemp, &lv_font_montserrat_36, LV_PART_MAIN);

	ui_LowerLimit = lv_label_create(ui_Temp);
	lv_obj_set_width(ui_LowerLimit, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height(ui_LowerLimit, LV_SIZE_CONTENT); /// 1
	lv_obj_set_x(ui_LowerLimit, -62);
	lv_obj_set_y(ui_LowerLimit, 48);
	lv_obj_set_align(ui_LowerLimit, LV_ALIGN_CENTER);
	lv_label_set_text(ui_LowerLimit, "0C");
	lv_obj_set_style_text_font(ui_LowerLimit, &lv_font_montserrat_32, LV_PART_MAIN);

	ui_UpperLimit = lv_label_create(ui_Temp);
	lv_obj_set_width(ui_UpperLimit, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height(ui_UpperLimit, LV_SIZE_CONTENT); /// 1
	lv_obj_set_x(ui_UpperLimit, 53);
	lv_obj_set_y(ui_UpperLimit, 48);
	lv_obj_set_align(ui_UpperLimit, LV_ALIGN_CENTER);
	lv_label_set_text(ui_UpperLimit, "1350C");
	lv_obj_set_style_text_font(ui_UpperLimit, &lv_font_montserrat_32, LV_PART_MAIN);

	ui_OnOff = lv_button_create(ui_Temp);
	lv_obj_set_width(ui_OnOff, 180);
	lv_obj_set_height(ui_OnOff, 64);
	lv_obj_set_x(ui_OnOff, -2);
	lv_obj_set_y(ui_OnOff, 110);
	lv_obj_set_align(ui_OnOff, LV_ALIGN_CENTER);

	// Fix flag conversion issue
	lv_obj_remove_flag(ui_OnOff, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE |
												 LV_OBJ_FLAG_SCROLL_ELASTIC |
												 LV_OBJ_FLAG_SCROLL_MOMENTUM |
												 LV_OBJ_FLAG_SCROLL_CHAIN));

	lv_obj_set_style_bg_color(ui_OnOff, lv_color_hex(onOffStoppedColor), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(ui_OnOff, 255, LV_PART_MAIN);

	ui_OnOffButtonLabel = lv_label_create(ui_OnOff);
	lv_obj_set_width(ui_OnOffButtonLabel, LV_SIZE_CONTENT);	 /// 1
	lv_obj_set_height(ui_OnOffButtonLabel, LV_SIZE_CONTENT); /// 1
	lv_obj_set_align(ui_OnOffButtonLabel, LV_ALIGN_CENTER);
	lv_label_set_text(ui_OnOffButtonLabel, " Start");
	lv_obj_set_style_text_font(ui_OnOffButtonLabel, &lv_font_montserrat_32, LV_PART_MAIN);

	lv_obj_add_event_cb(ui_Arc1, ui_event_Arc1, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_OnOff, ui_event_OnOff, LV_EVENT_CLICKED, NULL);

	SetCurrentTemp(currentTemp);
	SetTargetTemp(setTemp);
	UpdateLowerLimit(lowerLimit);
	UpdateUpperLimit(upperLimit);

	printf("end ui_Temp_screen_init\n");

	lv_disp_load_scr(ui_Temp);

	ESP_ERROR_CHECK(lcd_display_brightness_set(75));
}

TempUI::~TempUI()
{
	// Add cleanup code if necessary
}

void TempUI::ui_event_Arc1(lv_event_t *e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
	lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));

	if (event_code == LV_EVENT_VALUE_CHANGED)
	{
		TempUI *tempUI = TempUI::GetInstance();
		int temp = lv_arc_get_value(target);
		printf("Arc1 value changed: %d\n", temp);
		if (tempUI->setTemp != temp)
		{
			tempUI->setTemp = temp;
			tempUI->SetTargetTemp(tempUI->setTemp);
		}

		if (tempUI->onSetTempChanged)
		{
			tempUI->onSetTempChanged(e);
		}
	}
}

void TempUI::ui_event_OnOff(lv_event_t *e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
	if (event_code == LV_EVENT_CLICKED)
	{
		TempUI *tempUI = TempUI::GetInstance();
		printf("OnToggleStartStop\n");
		lvgl_port_lock(0);
		if (tempUI->started)
		{
			tempUI->started = false;
			lv_label_set_text(tempUI->ui_OnOffButtonLabel, "Start");
			lv_obj_set_style_bg_color(tempUI->ui_OnOff, lv_color_hex(onOffStoppedColor), LV_PART_MAIN);
		}
		else
		{
			tempUI->started = true;
			lv_label_set_text(tempUI->ui_OnOffButtonLabel, "Stop");
			lv_obj_set_style_bg_color(tempUI->ui_OnOff, lv_color_hex(onOffRunningColor), LV_PART_MAIN);
		}
		lvgl_port_unlock();

		if (tempUI->onToggleStartStop)
		{
			tempUI->onToggleStartStop(e);
		}
	}
}
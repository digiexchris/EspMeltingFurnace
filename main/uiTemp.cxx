#include "uiTemp.hxx"
#include "SPIBus.hxx"
#include "esp_check.h"
#include "esp_log.h"
#include "lcd.h"
#include "touch.h"
#include <esp_lcd_touch.h>
#include <esp_lcd_touch_xpt2046.h>
#include <stdio.h>

static const char *TAG = "TempUI";

esp_lcd_touch_handle_t TempUI::tp = NULL;
TaskHandle_t updateTaskHandle = NULL;

// Define the static instance variable
TempUI *TempUI::instance = nullptr;

TempUI::TempUI(SPIBusManager *aBusManager)
	: myBusManager(aBusManager), lcd_io(nullptr), lcd_panel(nullptr), lvgl_display(nullptr), ui_Temp(nullptr), ui_Arc1(nullptr), ui_CurrentTemp(nullptr), ui_SetTemp(nullptr), ui_LowerLimit(nullptr), ui_UpperLimit(nullptr), ui_OnOff(nullptr), ui_OnOffButtonLabel(nullptr)
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

	// ESP_ERROR_CHECK(touch_init(SPI3_HOST, myBusManager->getMutex(), &tp));
	// touch_cfg.disp = lvgl_display;
	// touch_cfg.handle = tp;

	// esp_lcd_panel_io_handle_t tp_io_handle = NULL;

	// const esp_lcd_panel_io_spi_config_t tp_io_config = {.cs_gpio_num = TOUCH_CS,
	// 													.dc_gpio_num = TOUCH_DC,
	// 													.spi_mode = 0,
	// 													.pclk_hz = TOUCH_CLOCK_HZ,
	// 													.trans_queue_depth = 3,
	// 													.on_color_trans_done = NULL,
	// 													.user_ctx = NULL,
	// 													.lcd_cmd_bits = 8,
	// 													.lcd_param_bits = 8,
	// 													.flags = {.dc_low_on_data = 0, .octal_mode = 0, .sio_mode = 0, .lsb_first = 0, .cs_high_active = 0}};
	// esp_lcd_touch_config_t tp_cfg = {
	// 	.x_max = LCD_H_RES,
	// 	.y_max = LCD_V_RES,
	// 	.rst_gpio_num = TOUCH_RST,
	// 	//.int_gpio_num = TOUCH_IRQ,
	// 	.levels = {.reset = 0, .interrupt = 0},
	// 	.flags =
	// 		{
	// 			.swap_xy = false,
	// 			.mirror_x = LCD_MIRROR_X,
	// 			.mirror_y = LCD_MIRROR_Y,
	// 		},
	// 	.process_coordinates = ProcessTouchCoordinates,
	// 	.interrupt_callback = NULL};

	// ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &tp_io_config, &tp_io_handle));
	// ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));

	// ESP_ERROR_CHECK(esp_lcd_touch_xpt2046_set_spi_mutex(myBusManager->getMutex()));

	// lv_indev_t *indev = lv_indev_create();
	// lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	// lv_indev_set_read_cb(indev, TouchDriverRead);
	// lv_indev_enable(indev, true); // Enable the input device to process touch events

	// Add touch to LVGL and store the resulting indev
	// lv_indev_t *touch_indev = lvgl_port_add_touch(&touch_cfg);
	// if (touch_indev == NULL)
	// {
	// 	ESP_LOGE(TAG, "Failed to add touch to LVGL");
	// }
	// else
	// {
	// 	ESP_LOGI(TAG, "Touch successfully added to LVGL");
	// }

	// Initialize rotary encoder and toggle switch
	// InitRotaryEncoder();
	// InitToggleSwitch();

	lvgl_port_lock(0);

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
	lv_arc_set_value(ui_Arc1, 150);
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

	// Turn on the backlight at maximum brightness
	ESP_LOGI(TAG, "Turning on backlight");
	ESP_ERROR_CHECK(lcd_display_brightness_set(75));
	lv_task_handler();
	lvgl_port_unlock();

	// xTaskCreate(UpdateTouchTask, "toggle_switch_task", 2 * 2048, this, 5, &updateTaskHandle);
}

TempUI::~TempUI()
{
	if (updateTaskHandle != NULL)
	{
		vTaskDelete(updateTaskHandle);
	}

	// Clean up PCNT resources
	if (pcnt_unit)
	{
		pcnt_unit_stop(pcnt_unit);
		pcnt_unit_disable(pcnt_unit);

		// Delete channels
		if (pcnt_chan_a)
		{
			pcnt_del_channel(pcnt_chan_a);
		}

		if (pcnt_chan_b)
		{
			pcnt_del_channel(pcnt_chan_b);
		}

		// Delete unit
		pcnt_del_unit(pcnt_unit);
	}
}

void TempUI::ui_event_Arc1(lv_event_t *e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
	lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
	ESP_LOGI(TAG, "ui_event_Arc1 event_code: %d", event_code);
	if (event_code == LV_EVENT_VALUE_CHANGED)
	{
		TempUI *tempUI = TempUI::GetInstance();
		int temp = lv_arc_get_value(target);
		printf("Arc1 value changed: %d\n", temp);
		if (tempUI->setTemp != temp)
		{
			if (tempUI->started)
			{
				tempUI->started = false;
			}
			tempUI->setTemp = temp;
			tempUI->SetTargetTemp(tempUI->setTemp);
		}
	}
}

void TempUI::ui_event_OnOff(lv_event_t *e)
{
	lv_event_code_t event_code = lv_event_get_code(e);

	switch (event_code)
	{
	case LV_EVENT_RELEASED:
	{
		TempUI *tempUI = TempUI::GetInstance();

		if (tempUI->started)
		{
			lvgl_port_lock(0);
			tempUI->started = false;
			lv_label_set_text(tempUI->ui_OnOffButtonLabel, "STOPPED");
			lv_obj_set_style_bg_color(tempUI->ui_OnOff, lv_color_hex(onOffStoppedColor), LV_PART_MAIN);
			lvgl_port_unlock();
		}
	}
	break;
	case LV_EVENT_PRESSED:
	{
		TempUI *tempUI = TempUI::GetInstance();

		if (!tempUI->started)
		{
			lvgl_port_lock(0);
			tempUI->started = true;
			lv_label_set_text(tempUI->ui_OnOffButtonLabel, "RUNNING");
			lv_obj_set_style_bg_color(tempUI->ui_OnOff, lv_color_hex(onOffRunningColor), LV_PART_MAIN);
			lvgl_port_unlock();
		}
	}

	break;
	default:
		break;
	}
}

void TempUI::Stop()
{
	lvgl_port_lock(0);
	started = false;
	lv_label_set_text(ui_OnOffButtonLabel, "STOPPED");
	lv_obj_set_style_bg_color(ui_OnOff, lv_color_hex(onOffStoppedColor), LV_PART_MAIN);
	lvgl_port_unlock();
}

void TempUI::Start()
{
	lvgl_port_lock(0);
	started = true;
	lv_label_set_text(ui_OnOffButtonLabel, "RUNNING");
	lv_obj_set_style_bg_color(ui_OnOff, lv_color_hex(onOffRunningColor), LV_PART_MAIN);
	lvgl_port_unlock();
}

void TempUI::UpdateTouchTask(void *param)
{
	TempUI *ui = static_cast<TempUI *>(param);

	while (true)
	{
		// Check for task notifications that would wake us up early
		uint32_t notification_value = 0;
		if (xTaskNotifyWait(0, ULONG_MAX, &notification_value, 0) == pdTRUE)
		{
			ESP_LOGI(TAG, "Task notification received: %lu", notification_value);
			// Handle different notification types if needed
			// For example: if (notification_value & NOTIFY_TEMP_CHANGED) { ... }
		}

		// /esp_lcd_touch_read_data(tp);

		// ESP_LOGI(TAG, "Touch read data");
		//  Short delay
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
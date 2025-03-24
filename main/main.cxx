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

#include "uiTemp.hxx"

#define TAG "MeltingFurnace"

extern "C" void app_main(void)
{
	TempUI *ui = new TempUI(
		[](lv_event_t *e)
		{
			printf("SetTempChanged\n");
		},
		[](lv_event_t *e)
		{
			printf("ToggleStartStop\n");
		});

	while (42)
	{
		// sprintf(buf, "%04d", n++);

		vTaskDelay(125 / portTICK_PERIOD_MS);
	}
	vTaskDelay(portMAX_DELAY);
}
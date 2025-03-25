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
#include "Adafruit_MAX31856.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "spi_port.h"

/* available pins:
CDS sensor: 34
TF SPI3: CS: 5, MOSI: 23, MISO: 19, CLK: 18
RGB LED: 4, 16, 17
Speaker: 26


Using SPI3 for the MAX31856 thermocouple and TF reader, with 27 on the left pin header for CS for the TF reader.
 * CS: 27
 * MOSI: 23 ( pin 37 on the SOC )
 * MISO: 19 ( pin 31 on the SOC )
 * CLK: 18 ( pin 30 on the SOC )
 *
 *
 *
 Using Pin 35 (left pin header) for the SSR output.
 */

Adafruit_MAX31856 thermocouple;

int temp = 0;

static void temp_task(void *args)
{
	const int NUM_SAMPLES = 10;
	float samples[NUM_SAMPLES];
	int sampleIndex = 0;

	while (1)
	{
		vTaskDelay(100);

		// check for conversion complete and read temperature
		if (thermocouple.conversionComplete())
		{
			// Add sample to array
			samples[sampleIndex] = thermocouple.readCJTemperature();
			sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;

			// Calculate average after collecting enough samples
			if (sampleIndex == 0 || sampleIndex >= NUM_SAMPLES)
			{
				float sum = 0;
				for (int i = 0; i < NUM_SAMPLES; i++)
				{
					sum += samples[i];
				}
				temp = (int)round(sum / NUM_SAMPLES);
				ESP_LOGI(TAG, "Avg Temp:%d", temp);
			}
		}
		else
		{
			ESP_LOGI(TAG, "Conversion not complete!");
		}
	}
}

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

	ESP_ERROR_CHECK(spi_master_init());
	ESP_LOGI(TAG, "SPI initialized successfully");

	ESP_ERROR_CHECK(thermocouple.begin(spi_read, spi_write, spi_dev));
	thermocouple.setThermocoupleType(MAX31856_TCTYPE_K);
	thermocouple.setConversionMode(MAX31856_CONTINUOUS);
	thermocouple.setNoiseFilter(MAX31856_NOISE_FILTER_60HZ);
	ESP_LOGI(TAG, "Thermocouple initialized successfully");

	xTaskCreate(temp_task, "App/temp", 4 * 1024, NULL, 10, NULL);

	while (42)
	{
		printf("Temp:%d\n", temp);

		vTaskDelay(125 / portTICK_PERIOD_MS);
	}
	vTaskDelay(portMAX_DELAY);
}
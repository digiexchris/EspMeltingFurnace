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
 Using Pin 35 is input only.
 */

gpio_num_t SSR_PIN = GPIO_NUM_26;

Adafruit_MAX31856 thermocouple;

int temp = 0;

bool heating = false;
bool errored = false;

float readTemp()
{
	thermocouple.triggerOneShot();
	uint64_t start = esp_timer_get_time();
	while (!thermocouple.conversionComplete())
	{
		if ((esp_timer_get_time() - start) / 1000 > 250) // convert us to ms
			return 0;
		vTaskDelay(10); // delay for 10ms
	}

	uint8_t fault = thermocouple.readFault();
	if (fault)
	{
		switch (fault)
		{
		case MAX31856_FAULT_CJRANGE:
		{
			float temp = thermocouple.readCJTemperature();
			ESP_LOGI(TAG, "Cold Junction Range Fault: %.2f", temp);
		}
		break;
		case MAX31856_FAULT_TCRANGE:
		{
			float temp = thermocouple.readThermocoupleTemperature();
			ESP_LOGI(TAG, "Thermocouple Range Fault: %.2f", temp);
		}
		break;
		case MAX31856_FAULT_CJHIGH:
		{
			float temp = thermocouple.readCJTemperature();
			ESP_LOGI(TAG, "Cold Junction High Fault: %.2f", temp);
		}
		break;
		case MAX31856_FAULT_CJLOW:
		{
			float temp = thermocouple.readCJTemperature();
			ESP_LOGI(TAG, "Cold Junction Low Fault: %.2f", temp);
		}
		break;
		case MAX31856_FAULT_TCHIGH:
		{
			float temp = thermocouple.readThermocoupleTemperature();
			ESP_LOGI(TAG, "Thermocouple High Fault: %.2f", temp);
		}
		break;
		case MAX31856_FAULT_TCLOW:
		{
			float temp = thermocouple.readThermocoupleTemperature();
			ESP_LOGI(TAG, "Thermocouple Low Fault: %.2f", temp);
		}
		break;
		case MAX31856_FAULT_OVUV:
			ESP_LOGI(TAG, "Over/Under Voltage Fault");
			break;
		case MAX31856_FAULT_OPEN:
			ESP_LOGI(TAG, "Thermocouple Open Fault");
			break;
		default:
			ESP_LOGI(TAG, "Unknown Fault: 0x%02x", fault);
			break;
		}

		return -9999;
	}
	else
	{
		// Add sample to array
		float temp = thermocouple.readThermocoupleTemperature();
		return temp;
	}
}

bool isInRange(int value, int min, int max)
{
	return value > min && value < max;
}

static void temp_task(void *args)
{
	const int NUM_SAMPLES = 10;
	float samples[NUM_SAMPLES];
	int sampleIndex = 0;

	while (1)
	{
		vTaskDelay(100 * portTICK_PERIOD_MS);

		int temp = static_cast<int>(readTemp());

		if (temp != -9999)
		{
			samples[sampleIndex] = temp;
			sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;

			float sum = 0;
			for (int i = 0; i < NUM_SAMPLES; i++)
			{
				sum += samples[i];
			}
			float average = sum / NUM_SAMPLES;

			if (fabs(average - temp) > 5)
			{
				temp = static_cast<int>(average);
			}
		}
		else
		{
			printf("Error reading temperature\n");
			errored = true;
		}
	}
}

extern "C" void app_main(void)
{
	TempUI *ui = new TempUI();

	ESP_ERROR_CHECK(spi_master_init());
	ESP_LOGI(TAG, "SPI initialized successfully");

	ESP_ERROR_CHECK(thermocouple.begin(spi_read, spi_write, spi_dev));
	thermocouple.setThermocoupleType(MAX31856_TCTYPE_K);
	thermocouple.setConversionMode(MAX31856_ONESHOT_NOWAIT);
	thermocouple.setNoiseFilter(MAX31856_NOISE_FILTER_60HZ);

	// set thermocouple internal chip temp thresholds
	thermocouple.setColdJunctionFaultThreshholds(0, 100);
	thermocouple.setTempFaultThreshholds(5, 1360);
	ESP_LOGI(TAG, "Thermocouple initialized successfully");

	xTaskCreate(temp_task, "App/temp", 4 * 1024, NULL, 10, NULL);

	// Configure GPIO 26 as an output for SSR control
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;		  // Disable interrupt
	io_conf.mode = GPIO_MODE_OUTPUT;			  // Set as output mode
	io_conf.pin_bit_mask = (1ULL << SSR_PIN);	  // Bit mask for pin 35
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;	  // Disable pull-up
	ESP_ERROR_CHECK(gpio_config(&io_conf));
	ESP_LOGI(TAG, "GPIO 26 configured as output for SSR control");

	// Ensure the heater is initially off
	gpio_set_level(SSR_PIN, 0);

	int lastTemp = -1;

	while (42)
	{
		if (errored)
		{
			ESP_LOGI(TAG, "Error reading temperature");
			ui->SetError(true);
		}

		if (temp != lastTemp)
		{
			lastTemp = temp;
			ESP_LOGI(TAG, "Temperature: %d", temp);
			ui->SetCurrentTemp(temp);
		}

		if (ui->IsStarted() && !errored && isInRange(temp, 5, 1360))
		{
			if (ui->IsError())
			{
				ui->SetError(false);
			}
			// run pid
		}
		else
		{
			gpio_set_level(SSR_PIN, 0);
			ESP_LOGI(TAG, "Heater OFF if either not started or errored or not in range");
			ui->SetError(true);
		}

		vTaskDelay(1000 * portTICK_PERIOD_MS);
	}
	vTaskDelay(portMAX_DELAY);
}
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
 * Using 26 (spkeaker) for the SSR control.l
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
	// Trigger a one-shot conversion
	thermocouple.triggerOneShot();

	// Wait for conversion to complete (max 250ms)
	uint32_t start = esp_timer_get_time() / 1000;
	while (!thermocouple.conversionComplete())
	{
		if ((esp_timer_get_time() / 1000) - start > 250)
		{
			ESP_LOGE(TAG, "Temperature conversion timeout");
			return -9999;
		}
		vTaskDelay(5 / portTICK_PERIOD_MS);
	}

	// Read the temperature
	float temp = thermocouple.readThermocoupleTemperature();
	float cj_temp = thermocouple.readCJTemperature();

	// Log temperature readings
	ESP_LOGD(TAG, "Thermocouple Temperature: %.2f°C", temp);
	ESP_LOGD(TAG, "Cold Junction Temperature: %.2f°C", cj_temp);

	// Check for any faults
	uint8_t fault = thermocouple.readFault();
	if (fault)
	{
		ESP_LOGW(TAG, "Fault detected (0x%02X):", fault);

		if (fault == 0xFF)
		{
			ESP_LOGE(TAG, "Communication error - all bits set (0xFF)");
			return -9999;
		}

		if (fault & MAX31856_FAULT_CJRANGE)
		{
			ESP_LOGW(TAG, "Cold Junction Range Fault: %.2f°C", cj_temp);
		}
		if (fault & MAX31856_FAULT_TCRANGE)
		{
			ESP_LOGW(TAG, "Thermocouple Range Fault: %.2f°C", temp);
		}
		if (fault & MAX31856_FAULT_CJHIGH)
		{
			ESP_LOGW(TAG, "Cold Junction High Fault: %.2f°C", cj_temp);
		}
		if (fault & MAX31856_FAULT_CJLOW)
		{
			ESP_LOGW(TAG, "Cold Junction Low Fault: %.2f°C", cj_temp);
		}
		if (fault & MAX31856_FAULT_TCHIGH)
		{
			ESP_LOGW(TAG, "Thermocouple High Fault: %.2f°C", temp);
		}
		if (fault & MAX31856_FAULT_TCLOW)
		{
			ESP_LOGW(TAG, "Thermocouple Low Fault: %.2f°C", temp);
		}
		if (fault & MAX31856_FAULT_OVUV)
		{
			ESP_LOGW(TAG, "Over/Under Voltage Fault");
		}
		if (fault & MAX31856_FAULT_OPEN)
		{
			ESP_LOGW(TAG, "Thermocouple Open Fault");
		}

		// If it's a range fault or voltage fault, we can still use the temperature
		if (fault & (MAX31856_FAULT_OPEN | MAX31856_FAULT_OVUV))
		{
			return -9999;
		}
	}

	// If temperature is extremely out of range, treat as an error
	if (temp < -100 || temp > 2000)
	{
		ESP_LOGE(TAG, "Temperature reading out of realistic range: %.2f°C", temp);
		return -9999;
	}

	return temp;
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

		auto ftemp = readTemp();
		ESP_LOGD(TAG, "Read temperature: %.2f", ftemp);

		int temp = static_cast<int>(readTemp());

		ESP_LOGD(TAG, "Casted Temp = %d", temp);

		// if (temp != -9999)
		// {
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
	// else
	// {
	// 	printf("Error reading temperature\n");
	// 	errored = true;
	// }
}

extern "C" void app_main(void)
{
	TempUI *ui = new TempUI();

	ESP_ERROR_CHECK(spi_master_init());
	ESP_LOGI(TAG, "SPI initialized successfully");

	ESP_ERROR_CHECK(thermocouple.begin(spi_read, spi_write, spi_dev));
	ESP_LOGI(TAG, "MAX31856 begin successful");

	// Read fault register after initialization
	uint8_t fault = thermocouple.readFault();
	ESP_LOGI(TAG, "Initial fault register: 0x%02X", fault);

	thermocouple.setThermocoupleType(MAX31856_TCTYPE_K);
	ESP_LOGI(TAG, "Thermocouple type set to K");

	thermocouple.setConversionMode(MAX31856_ONESHOT);
	ESP_LOGI(TAG, "Conversion mode set to one-shot");

	thermocouple.setNoiseFilter(MAX31856_NOISE_FILTER_60HZ);
	ESP_LOGI(TAG, "Noise filter set to 60Hz");

	// Read fault register again
	fault = thermocouple.readFault();
	ESP_LOGI(TAG, "Fault register after setup: 0x%02X", fault);

	ESP_LOGD(TAG, "Thermocouple type: %s",
			 [&]() -> const char *
			 {
				 switch (thermocouple.getThermocoupleType())
				 {
				 case MAX31856_TCTYPE_B:
					 return "B Type";
				 case MAX31856_TCTYPE_E:
					 return "E Type";
				 case MAX31856_TCTYPE_J:
					 return "J Type";
				 case MAX31856_TCTYPE_K:
					 return "K Type";
				 case MAX31856_TCTYPE_N:
					 return "N Type";
				 case MAX31856_TCTYPE_R:
					 return "R Type";
				 case MAX31856_TCTYPE_S:
					 return "S Type";
				 case MAX31856_TCTYPE_T:
					 return "T Type";
				 case MAX31856_VMODE_G8:
					 return "Voltage x8 Gain mode";
				 case MAX31856_VMODE_G32:
					 return "Voltage x32 Gain mode";
				 default:
					 return "Unknown";
				 }
			 }());

	// set thermocouple internal chip temp thresholds with wider range
	thermocouple.setColdJunctionFaultThreshholds(-20, 120); // Wider range for CJ
	ESP_LOGI(TAG, "Cold junction thresholds set to -20°C to 120°C");

	thermocouple.setTempFaultThreshholds(0, 1400); // Wider range for thermocouple
	ESP_LOGI(TAG, "Thermocouple thresholds set to 0°C to 1400°C");

	// Read fault register after thresholds
	fault = thermocouple.readFault();
	ESP_LOGI(TAG, "Fault register after thresholds: 0x%02X", fault);
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
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
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "tempController.hxx"

#include "SPIBus.hxx"

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

gpio_num_t SSR_PIN = GPIO_NUM_22;

const int SSR_FULL_PWM = 1023;
const int SSR_HALF_PWM = 512;
const int SSR_OFF_PWM = 0;

int SSR_CURRENT_PWM = SSR_OFF_PWM;

bool isInRange(int value, int min, int max)
{
	return value > min && value < max;
}

void setSSRDutyCycle(int duty)
{
	// ensure it's in range
	if (duty < 0)
	{
		duty = 0;
	}
	else if (duty > SSR_FULL_PWM)
	{
		duty = SSR_FULL_PWM;
	}
	ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

	SSR_CURRENT_PWM = duty;
}

extern "C" void app_main(void)
{
	SPIBusManager *spi3Manager = new SPIBusManager(SPI3_HOST);
	TempUI *ui = new TempUI(spi3Manager);
	TempController controller(ui, spi3Manager);

	// Configure GPIO 26 as an output for SSR control
	// Configure LEDC timer for 1Hz PWM frequency
	ledc_timer_config_t timer_conf = {};
	timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
	timer_conf.timer_num = LEDC_TIMER_0;
	timer_conf.duty_resolution = LEDC_TIMER_10_BIT; // 10-bit resolution (0-1023)
	timer_conf.freq_hz = 1;							// 1 Hz frequency
	ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

	// Configure LEDC channel
	ledc_channel_config_t channel_conf = {};
	channel_conf.gpio_num = SSR_PIN;
	channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
	channel_conf.channel = LEDC_CHANNEL_0;
	channel_conf.intr_type = LEDC_INTR_DISABLE;
	channel_conf.timer_sel = LEDC_TIMER_0;
	channel_conf.duty = 0; // Start with 0% duty cycle
	channel_conf.hpoint = 0;
	ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
	ESP_LOGI(TAG, "GPIO 22 configured as output for SSR control");

	// Ensure the heater is initially off
	setSSRDutyCycle(SSR_OFF_PWM);

	vTaskDelay(1000 / portTICK_PERIOD_MS);

	MAX31856::Result result = controller.GetLastResult();

	while (42)
	{
		// figure it out later, but the lvgl loop is not polling this correctly so I'm doing it here
		esp_lcd_touch_read_data(TempUI::tp);

		result = controller.GetLastResult();

		if (isInRange(result.thermocouple_c, 5, 1360) && isInRange(result.coldjunction_c, 5, 100))
		{
			if (ui->IsError())
			{
				ui->SetError(false);
			}

			if (ui->IsStarted())
			{
				if (result.thermocouple_c < ui->GetTargetTemp())
				{
					if (SSR_CURRENT_PWM != SSR_HALF_PWM)
					{
						setSSRDutyCycle(SSR_HALF_PWM);
						ESP_LOGI(TAG, "Heater ON");
					}
				}
				else
				{
					if (SSR_CURRENT_PWM != SSR_OFF_PWM)
					{
						setSSRDutyCycle(SSR_OFF_PWM);
						ESP_LOGI(TAG, "Heater OFF");
					}
				}
			}
			else
			{
				if (SSR_CURRENT_PWM != SSR_OFF_PWM)
				{
					setSSRDutyCycle(SSR_OFF_PWM);
					ESP_LOGI(TAG, "Heater OFF if not started");
				}
			}
			// run pid
		}
		else
		{
			ui->SetError(true);
			if (SSR_CURRENT_PWM != SSR_OFF_PWM)
			{
				setSSRDutyCycle(SSR_OFF_PWM);
				ESP_LOGI(TAG, "Heater OFF if either not started or errored or not in range");
			}
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	vTaskDelay(portMAX_DELAY);
}
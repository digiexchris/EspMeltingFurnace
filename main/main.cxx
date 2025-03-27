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

gpio_num_t SSR_PIN = GPIO_NUM_26;

int temp = 0;

bool heating = false;
bool errored = false;

bool isInRange(int value, int min, int max)
{
	return value > min && value < max;
}

extern "C" void app_main(void)
{
	SPIBusManager *spi3Manager = new SPIBusManager(SPI3_HOST);
	TempUI *ui = new TempUI(spi3Manager);
	TempController controller(ui, spi3Manager);

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

	// int lastTemp = -1;

	vTaskDelay(1000 / portTICK_PERIOD_MS);

	while (42)
	{
		// figure it out later, but the lvgl loop is not polling this correctly so I'm doing it here
		esp_lcd_touch_read_data(TempUI::tp);
		//  if (errored)
		//  {
		//  	ESP_LOGI(TAG, "Error reading temperature");
		//  	ui->SetError(true);
		//  }

		// if (temp != lastTemp)
		// {
		// 	lastTemp = temp;
		// 	ESP_LOGI(TAG, "Temperature: %d", temp);
		// 	ui->SetCurrentTemp(temp);
		// }

		// if (ui->IsStarted() && !errored && isInRange(temp, 5, 1360))
		// {
		// 	if (ui->IsError())
		// 	{
		// 		ui->SetError(false);
		// 	}
		// 	// run pid
		// }
		// else
		// {
		// 	gpio_set_level(SSR_PIN, 0);
		// 	ESP_LOGI(TAG, "Heater OFF if either not started or errored or not in range");
		// 	ui->SetError(true);
		// }

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	vTaskDelay(portMAX_DELAY);
}
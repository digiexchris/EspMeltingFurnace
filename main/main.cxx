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

// extern "C"
// {
// #include "lcd.h"
// #include "touch.h"
// }

#include "uiTemp.hxx"

#define TAG "MeltingFurnace"
#include "tempController.hxx"

#include "SPIBus.hxx"

#include "GPIO.hxx"

#include "hardware.h"

#include "driver/i2c_master.h"
#include "sdkconfig.h"
// #include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <vector>
extern "C" void scanI2CBus()
{
	// 	ESP_LOGI("SCANNER", "Scanning I2C bus for devices...");

	// 	std::vector<uint8_t> i2c_devices;

	// 	i2c_master_bus_config_t i2c_mst_config = {

	// 		.i2c_port = I2C_NUM_0,
	// 		.sda_io_num = I2C_SDA,
	// 		.scl_io_num = I2C_SCL,
	// 		.clk_source = I2C_CLK_SRC_DEFAULT,
	// 		.glitch_ignore_cnt = 7,
	// 		.flags = {
	// 			.enable_internal_pullup = true,
	// 		}};

	// 	i2c_master_bus_handle_t bus_handle;
	// 	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

	// 	uint16_t i;
	// 	esp_err_t espRc;
	// 	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	// 	printf("00:         ");
	// 	for (i = 3; i < 0x78; i++)
	// 	{
	// 		i2c_device_config_t dev_cfg = {
	// 			.dev_addr_length = I2C_ADDR_BIT_LEN_7,
	// 			.device_address = i,
	// 			.scl_speed_hz = 100000,
	// 		};

	// 		i2c_master_dev_handle_t dev_handle;
	// 		ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

	// #define DATA_LENGTH 100

	// 		auto ret = i2c_master_transmit(dev_handle, (const uint8_t *)"asdf", DATA_LENGTH, -1);
	// 		if (ret == ESP_OK)
	// 		{
	// 			printf(" %.2x", i);
	// 			i2c_devices.push_back(i);
	// 		}
	// 		else
	// 		{
	// 			printf(" --");
	// 		}
	// 		printf("\n");
	// 	}

	// 	for (auto device : i2c_devices)
	// 	{
	// 		printf("Found device at address: 0x%02X\n", device);
	// 	}

	// 	ESP_LOGI("SCANNER", "Expected MCP23017 address: 0x%02X", MCP23017_I2C_ADDR);
}

extern "C" void app_main(void)
{
	// scanI2CBus();
	SPIBusManager *spi3Manager = new SPIBusManager(SPI3_HOST);
	TempUI *ui = new TempUI(spi3Manager);
	TempController controller(ui, spi3Manager);

	GPIOManager *gpio = new GPIOManager();

	vTaskDelay(portMAX_DELAY);
}
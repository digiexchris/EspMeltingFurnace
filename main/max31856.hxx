#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "soc/gpio_struct.h"

#include <esp_log.h>

#define PIN_NUM_MISO CONFIG_PIN_NUM_MISO
#define PIN_NUM_MOSI CONFIG_PIN_NUM_MOSI
#define PIN_NUM_CLK CONFIG_PIN_NUM_CLK
#define PIN_NUM_CS CONFIG_PIN_NUM_CS

#define MAX31856_CR0_REG 0x00
#define MAX31856_CR0_AUTOCONVERT 0x80
#define MAX31856_CR0_1SHOT 0x40
#define MAX31856_CR0_OCFAULT1 0x20
#define MAX31856_CR0_OCFAULT0 0x10
#define MAX31856_CR0_CJ 0x08
#define MAX31856_CR0_FAULT 0x04
#define MAX31856_CR0_FAULTCLR 0x02

#define MAX31856_CR1_REG 0x01
#define MAX31856_MASK_REG 0x02
#define MAX31856_CJHF_REG 0x03
#define MAX31856_CJLF_REG 0x04
#define MAX31856_LTHFTH_REG 0x05
#define MAX31856_LTHFTL_REG 0x06
#define MAX31856_LTLFTH_REG 0x07
#define MAX31856_LTLFTL_REG 0x08
#define MAX31856_CJTO_REG 0x09
#define MAX31856_CJTH_REG 0x0A
#define MAX31856_CJTL_REG 0x0B
#define MAX31856_LTCBH_REG 0x0C
#define MAX31856_LTCBM_REG 0x0D
#define MAX31856_LTCBL_REG 0x0E
#define MAX31856_SR_REG 0x0F

#define MAX31856_FAULT_CJRANGE 0x80
#define MAX31856_FAULT_TCRANGE 0x40
#define MAX31856_FAULT_CJHIGH 0x20
#define MAX31856_FAULT_CJLOW 0x10
#define MAX31856_FAULT_TCHIGH 0x08
#define MAX31856_FAULT_TCLOW 0x04
#define MAX31856_FAULT_OVUV 0x02
#define MAX31856_FAULT_OPEN 0x01

enum class max31856_thermocoupletype_t
{
	MAX31856_TCTYPE_B = 0b0000,
	MAX31856_TCTYPE_E = 0b0001,
	MAX31856_TCTYPE_J = 0b0010,
	MAX31856_TCTYPE_K = 0b0011,
	MAX31856_TCTYPE_N = 0b0100,
	MAX31856_TCTYPE_R = 0b0101,
	MAX31856_TCTYPE_S = 0b0110,
	MAX31856_TCTYPE_T = 0b0111,
	MAX31856_VMODE_G8 = 0b1000,
	MAX31856_VMODE_G32 = 0b1100,
};

struct max31856_result_t
{
	spi_device_handle_t spi;
	float coldjunction_c;
	float coldjunction_f;
	float thermocouple_c;
	float thermocouple_f;
	uint8_t fault;
};

using MAX31856Callback = void (*)(max31856_result_t *result);

class MAX31856
{
public:
	MAX31856(max31856_thermocoupletype_t type, spi_host_device_t host_id, gpio_num_t cs_pin, spi_bus_config_t aBusConfig = defaultSpi3BusConfig);
	~MAX31856();

	max31856_thermocoupletype_t getType();
	void setTempFaultThreshholds(float low, float high);
	void setColdJunctionFaultThreshholds(float low, float high);
	uint8_t readFault(bool log_fault = false);
	float readColdJunction();
	float readTemperature();
	void setType(max31856_thermocoupletype_t type);
	void oneshotTemperature();

private:
	QueueHandle_t thermocouple_queue = NULL;
	spi_bus_config_t buscfg;
	spi_host_device_t host_id;
	gpio_num_t cs_pin;
	spi_device_handle_t spi_handle;
	max31856_result_t max31856_result;

	max31856_thermocoupletype_t thermocoupleType;

	static void receiverTask(void *pvParameter);
	static void thermocoupleTask(void *pvParameter);

	void writeRegister(uint8_t address, uint8_t data);
	uint8_t readRegister(uint8_t address);
	uint8_t readFastRegister(uint8_t address);
	uint16_t readRegister16(uint8_t address);
	uint32_t readRegister24(uint8_t address);

	static spi_bus_config_t defaultSpi3BusConfig;
};
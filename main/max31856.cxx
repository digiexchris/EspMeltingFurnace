#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

#include "max31856.hxx"

const char *TAG = "MAX31856";

spi_bus_config_t MAX31856::defaultSpi3BusConfig = {
	.mosi_io_num = GPIO_NUM_32,
	.miso_io_num = GPIO_NUM_39,
	.sclk_io_num = GPIO_NUM_25,
	.quadwp_io_num = -1,
	.quadhd_io_num = -1,
	.max_transfer_sz = 0,
};

MAX31856::MAX31856(max31856_thermocoupletype_t type, spi_host_device_t host_id, gpio_num_t cs_pin, spi_bus_config_t aBusConfig) : buscfg(aBusConfig), host_id(host_id), cs_pin(cs_pin), spi_handle(nullptr), thermocoupleType(type)
{
	ESP_LOGI(TAG, "Initialize");

	thermocouple_queue = xQueueCreate(5, sizeof(struct max31856_result_t));
	if (thermocouple_queue != NULL)
	{
		ESP_LOGI(TAG, "Created: thermocouple_queue");
	}
	else
	{
		ESP_LOGE(TAG, "Failed to Create: thermocouple_queue");
		assert(false);
	}

	gpio_config_t io_conf;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pin_bit_mask = (1ULL << this->cs_pin);
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);
	gpio_set_level(this->cs_pin, 1);

	vTaskDelay(100 / portTICK_PERIOD_MS);

	esp_err_t ret;

	spi_device_interface_config_t devcfg = {

		.dummy_bits = 0,
		.mode = 1,
		.clock_speed_hz = (APB_CLK_FREQ / 10), // 8 Mhz

		.spics_io_num = -1, // Manually Control CS
		.flags = 0,
		.queue_size = 1,
	};

	ret = spi_bus_initialize(HSPI_HOST, &buscfg, 0); // No DMA
	ESP_ERROR_CHECK(ret);
	ret = spi_bus_add_device(HSPI_HOST, &devcfg, &this->spi_handle);
	ESP_ERROR_CHECK(ret);

	// Assert on All Faults
	writeRegister(MAX31856_MASK_REG, 0x00);
	uint8_t cr0_val = readRegister(MAX31856_CR0_REG);
	cr0_val |= MAX31856_CR0_FAULTCLR; // Set the FAULTCLR bit
	writeRegister(MAX31856_CR0_REG, cr0_val);

	// Open Circuit Detection
	writeRegister(MAX31856_CR0_REG, MAX31856_CR0_OCFAULT0);

	max31856_result = {
		.spi = this->spi_handle,
	};

	xTaskCreate(&thermocoupleTask, "thermocouple_task", 2048, this, 5, NULL);
	xTaskCreate(&receiverTask, "receiver_task", 2048, this, 5, NULL);
}

void MAX31856::thermocoupleTask(void *pvParameter)
{
	ESP_LOGI("task", "Created: receiver_task");
	MAX31856 *instance = static_cast<MAX31856 *>(pvParameter);

	instance->setType(instance->thermocoupleType);

	while (42)
	{
		instance->readFault(true);
		instance->readColdJunction();
		instance->readTemperature();
		xQueueSend(instance->thermocouple_queue, &instance->max31856_result, (TickType_t)0);
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
}

void MAX31856::receiverTask(void *pvParameter)
{
	// TODO this probably belongs to main instead. Maybe call a callback from here passing the result back?
	ESP_LOGI("task", "Created: thermocouple_task");
	MAX31856 *instance = static_cast<MAX31856 *>(pvParameter);

	max31856_result_t max31856_result = {

	};

	while (1)
	{
		xQueueReceive(instance->thermocouple_queue, &max31856_result, (TickType_t)(200 / portTICK_PERIOD_MS));
		printf("Struct Received on Queue:\nCold Junction: %0.2f\nTemperature: %0.2f\n", max31856_result.coldjunction_f, max31856_result.thermocouple_f);
		printf("Fault: %d\n", max31856_result.fault);
		printf("Cold Junction: %0.2f\n", max31856_result.coldjunction_c);
		printf("Thermocouple: %0.2f\n", max31856_result.thermocouple_c);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

void MAX31856::setTempFaultThreshholds(float low, float high)
{
	low *= 16;
	high *= 16;
	int16_t low_int = low;
	int16_t high_int = high;
	writeRegister(MAX31856_LTHFTH_REG, high_int >> 8);
	writeRegister(MAX31856_LTHFTL_REG, high_int);
	writeRegister(MAX31856_LTLFTH_REG, low_int >> 8);
	writeRegister(MAX31856_LTLFTL_REG, low_int);
}

uint8_t MAX31856::readFault(bool log_fault)
{
	uint8_t fault_val = readRegister(MAX31856_SR_REG);
	if (log_fault && fault_val)
	{
		if (fault_val & MAX31856_FAULT_CJRANGE)
			ESP_LOGI(TAG, "Fault: Cold Junction Range");
		if (fault_val & MAX31856_FAULT_TCRANGE)
			ESP_LOGI(TAG, "Fault: Thermocouple Range");
		if (fault_val & MAX31856_FAULT_CJHIGH)
			ESP_LOGI(TAG, "Fault: Cold Junction High");
		if (fault_val & MAX31856_FAULT_CJLOW)
			ESP_LOGI(TAG, "Fault: Cold Junction Low");
		if (fault_val & MAX31856_FAULT_TCHIGH)
			ESP_LOGI(TAG, "Fault: Thermocouple High");
		if (fault_val & MAX31856_FAULT_TCLOW)
			ESP_LOGI(TAG, "Fault: Thermocouple Low");
		if (fault_val & MAX31856_FAULT_OVUV)
			ESP_LOGI(TAG, "Fault: Over/Under Voltage");
		if (fault_val & MAX31856_FAULT_OPEN)
			ESP_LOGI(TAG, "Fault: Thermocouple Open");

		uint8_t cr0_val = readRegister(MAX31856_CR0_REG);
		cr0_val |= MAX31856_CR0_FAULTCLR; // Set the FAULTCLR bit
		writeRegister(MAX31856_CR0_REG, cr0_val);
	}
	max31856_result.fault = fault_val;

	return fault_val;
}

float MAX31856::readColdJunction()
{
	oneshotTemperature();
	uint16_t cj_temp = readRegister16(MAX31856_CJTH_REG);
	float cj_temp_float = cj_temp;
	cj_temp_float /= 256.0;
	max31856_result.coldjunction_c = cj_temp_float;
	max31856_result.coldjunction_f = (1.8 * cj_temp_float) + 32.0;
	return cj_temp_float;
}

float MAX31856::readTemperature()
{
	oneshotTemperature();
	uint32_t tc_temp = readRegister24(MAX31856_LTCBH_REG);
	if (tc_temp & 0x800000)
	{
		tc_temp |= 0xFF000000; // fix sign bit
	}
	tc_temp >>= 5; // bottom 5 bits are unused
	float tc_temp_float = tc_temp;
	tc_temp_float *= 0.0078125;
	max31856_result.thermocouple_c = tc_temp_float;
	max31856_result.thermocouple_f = (1.8 * tc_temp_float) + 32.0;
	return tc_temp_float;
}

void MAX31856::setType(max31856_thermocoupletype_t type)
{
	thermocoupleType = type;
	// writeRegister(MAX31856_CR0_REG, MAX31856_CR0_OCFAULT0);
	// writeRegister(MAX31856_CR1_REG, (uint8_t)type);

	uint8_t val = readRegister(MAX31856_CR1_REG);
	val &= 0xF0; // Mask off bottom 4 bits
	val |= static_cast<uint8_t>(type) & 0x0F;
	writeRegister(MAX31856_CR1_REG, val);
}

void MAX31856::oneshotTemperature()
{
	// writeRegister(MAX31856_CR0_REG, MAX31856_CR0_OCFAULT0 | MAX31856_CR0_CJ);
	// uint8_t status = readRegister(MAX31856_SR_REG);
	// while (!(status & MAX31856_SR_REG))
	// {
	// 	status = readRegister(MAX31856_SR_REG);
	// 	vTaskDelay(5 / portTICK_PERIOD_MS);
	// }

	writeRegister(MAX31856_CJTO_REG, 0x00);
	uint8_t val = readRegister(MAX31856_CR0_REG);
	val &= ~MAX31856_CR0_AUTOCONVERT;
	val |= MAX31856_CR0_1SHOT;
	writeRegister(MAX31856_CR0_REG, val);
	vTaskDelay(250 / portTICK_PERIOD_MS);
}

max31856_thermocoupletype_t MAX31856::getType()
{
	uint8_t val = readRegister(MAX31856_CR1_REG);

	val &= 0x0F;

	max31856_thermocoupletype_t type = static_cast<max31856_thermocoupletype_t>(val);

	switch (type)
	{
	case max31856_thermocoupletype_t::MAX31856_TCTYPE_B:
		ESP_LOGI(TAG, "TC Type: B");
		break;
	case max31856_thermocoupletype_t::MAX31856_TCTYPE_E:
		ESP_LOGI(TAG, "TC Type: E");
		break;
	case max31856_thermocoupletype_t::MAX31856_TCTYPE_J:
		ESP_LOGI(TAG, "TC Type: J");
		break;
	case max31856_thermocoupletype_t::MAX31856_TCTYPE_K:
		ESP_LOGI(TAG, "TC Type: K");
		break;
	case max31856_thermocoupletype_t::MAX31856_TCTYPE_N:
		ESP_LOGI(TAG, "TC Type: N");
		break;
	case max31856_thermocoupletype_t::MAX31856_TCTYPE_R:
		ESP_LOGI(TAG, "TC Type: R");
		break;
	case max31856_thermocoupletype_t::MAX31856_TCTYPE_S:
		ESP_LOGI(TAG, "TC Type: S");
		break;
	case max31856_thermocoupletype_t::MAX31856_TCTYPE_T:
		ESP_LOGI(TAG, "TC Type: T");
		break;
	case max31856_thermocoupletype_t::MAX31856_VMODE_G8:
		ESP_LOGI(TAG, "Voltage x8 Gain mode");
		break;
	case max31856_thermocoupletype_t::MAX31856_VMODE_G32:
		ESP_LOGI(TAG, "Voltage x8 Gain mode");
		break;
	default:
		ESP_LOGI(TAG, "TC Type: Unknown");
		break;
	}

	return type;
}

void MAX31856::setColdJunctionFaultThreshholds(float low, float high)
{
	low *= 16;
	high *= 16;
	int16_t low_int = low;
	int16_t high_int = high;
	writeRegister(MAX31856_CJHF_REG, high_int >> 8);
	writeRegister(MAX31856_CJLF_REG, high_int);
	writeRegister(MAX31856_CJTH_REG, low_int >> 8);
	writeRegister(MAX31856_CJTL_REG, low_int);
}

void MAX31856::writeRegister(uint8_t address, uint8_t data)
{
	esp_err_t ret;
	spi_transaction_t spi_transaction;
	memset(&spi_transaction, 0, sizeof(spi_transaction_t));
	uint8_t tx_data[1] = {static_cast<uint8_t>(address | 0x80)};

	gpio_set_level(this->cs_pin, 0);
	spi_transaction.flags = SPI_TRANS_USE_RXDATA;
	spi_transaction.length = 8;
	spi_transaction.tx_buffer = tx_data;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);

	tx_data[0] = data;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);
	gpio_set_level(this->cs_pin, 1);
}

uint8_t MAX31856::readRegister(uint8_t address)
{
	esp_err_t ret;
	spi_transaction_t spi_transaction;
	memset(&spi_transaction, 0, sizeof(spi_transaction_t));
	uint8_t tx_data[1] = {static_cast<uint8_t>(address & 0x7F)};

	gpio_set_level(this->cs_pin, 0);
	spi_transaction.flags = SPI_TRANS_USE_RXDATA;
	spi_transaction.length = 8;
	spi_transaction.tx_buffer = tx_data;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);

	tx_data[0] = 0xFF;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);
	gpio_set_level(this->cs_pin, 1);
	uint8_t reg_value = spi_transaction.rx_data[0];
	return reg_value;
}

uint8_t MAX31856::readFastRegister(uint8_t address)
{
	esp_err_t ret;
	spi_transaction_t spi_transaction;
	memset(&spi_transaction, 0, sizeof(spi_transaction_t));
	uint8_t tx_data[2] = {static_cast<uint8_t>(address & 0x7F), 0xFF};

	gpio_set_level(this->cs_pin, 0);
	spi_transaction.flags = SPI_TRANS_USE_RXDATA;
	spi_transaction.length = 16;
	spi_transaction.tx_buffer = tx_data;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);
	gpio_set_level(this->cs_pin, 1);
	uint8_t reg_value = spi_transaction.rx_data[0];
	return reg_value;
}

uint16_t MAX31856::readRegister16(uint8_t address)
{
	esp_err_t ret;
	spi_transaction_t spi_transaction;
	memset(&spi_transaction, 0, sizeof(spi_transaction_t));
	uint8_t tx_data[1] = {static_cast<uint8_t>(address & 0x7F)};

	gpio_set_level(this->cs_pin, 0);
	spi_transaction.length = 8;
	spi_transaction.flags = SPI_TRANS_USE_RXDATA;
	spi_transaction.tx_buffer = tx_data;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);

	tx_data[0] = 0xFF;
	spi_transaction.length = 8;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);
	uint8_t b1 = spi_transaction.rx_data[0];

	spi_transaction.length = 8;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);
	uint8_t b2 = spi_transaction.rx_data[0];
	gpio_set_level(this->cs_pin, 1);

	uint16_t reg_value = ((b1 << 8) | b2);
	return reg_value;
}

uint32_t MAX31856::readRegister24(uint8_t address)
{
	esp_err_t ret;
	spi_transaction_t spi_transaction;
	memset(&spi_transaction, 0, sizeof(spi_transaction_t));
	uint8_t tx_data[1] = {static_cast<uint8_t>(address & 0x7F)};

	gpio_set_level(this->cs_pin, 0);
	spi_transaction.length = 8;
	spi_transaction.flags = SPI_TRANS_USE_RXDATA;
	spi_transaction.tx_buffer = tx_data;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);

	tx_data[0] = 0xFF;
	spi_transaction.length = 8;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);
	uint8_t b1 = spi_transaction.rx_data[0];

	tx_data[0] = 0xFF;
	spi_transaction.length = 8;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);
	uint8_t b2 = spi_transaction.rx_data[0];

	tx_data[0] = 0xFF;
	spi_transaction.length = 8;
	ret = spi_device_transmit(spi_handle, &spi_transaction);
	ESP_ERROR_CHECK(ret);
	uint8_t b3 = spi_transaction.rx_data[0];

	uint32_t reg_value = ((b1 << 16) | (b2 << 8) | b3);
	return reg_value;
}
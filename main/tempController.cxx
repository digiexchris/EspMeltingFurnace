#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

#include "max31856-espidf/max31856.hxx"

#include "tempController.hxx"

#include "driver/gpio.h"
#include "driver/ledc.h"

#include <esp_log.h>

const char *TCTAG = "TempController";

TempController::TempController(TempUI *aTempUi, SPIBusManager *aBusManager) : myTempUi(aTempUi), mySpiBusManager(aBusManager), myInstance(nullptr), myThermocoupleQueue(nullptr)
{
	ESP_LOGI(TCTAG, "Initialize");

	myInstance = this;

	assert(aBusManager != nullptr);
	assert(aTempUi != nullptr);

	mySpiBusManager->lock();
	myThermocouple = new MAX31856::MAX31856(SPI3_HOST, false);
	myThermocouple->AddDevice(MAX31856::ThermocoupleType::MAX31856_TCTYPE_K, GPIO_NUM_27, 0);
	mySpiBusManager->unlock();

	myThermocoupleQueue = xQueueCreate(5, sizeof(struct MAX31856::Result));
	if (myThermocoupleQueue != NULL)
	{
		ESP_LOGI(TCTAG, "Created: thermocouple_queue");
	}
	else
	{
		ESP_LOGE(TCTAG, "Failed to Create: thermocouple_queue");
		assert(false);
	}

	xTaskCreate(&thermocoupleTask, "thermocouple_task", 2048, this, 6, NULL);
	xTaskCreate(&receiverTask, "receiver_task", 2048, this, 6, NULL);

	myLastCurrentTemp = static_cast<double>(myLastResult.thermocouple_c);
	static int targetTemp = myTempUi->GetTargetTemp();
	myLastSetTemp = reinterpret_cast<double *>(&targetTemp);
	myRelayState = new bool(false);
}

void TempController::thermocoupleTask(void *pvParameter)
{
	ESP_LOGI("task", "Created: receiver_task");
	TempController *instance = static_cast<TempController *>(pvParameter);

	MAX31856::Result result = {
		.coldjunction_c = 0,
		.coldjunction_f = 0,
		.thermocouple_c = 0,
		.thermocouple_f = 0,
		.fault = 0,
	};

	while (42)
	{
		instance->mySpiBusManager->lock();
		instance->myThermocouple->read(result, 0);
		instance->mySpiBusManager->unlock();
		xQueueSend(instance->myThermocoupleQueue, &result, (TickType_t)0);
		vTaskDelay(600 / portTICK_PERIOD_MS);
	}
}

void TempController::receiverTask(void *pvParameter)
{
	// TODO this probably belongs to main instead. Maybe call a callback from here passing the result back?
	ESP_LOGI("task", "Created: thermocouple_task");
	TempController *instance = static_cast<TempController *>(pvParameter);

	MAX31856::Result result = {
		.coldjunction_c = 0,
		.coldjunction_f = 0,
		.thermocouple_c = 0,
		.thermocouple_f = 0,
		.fault = 0,
	};

	float lastTemp = 0;
	float lastColdTemp = 0;

	while (1)
	{
		xQueueReceive(instance->myThermocoupleQueue, &result, (TickType_t)(200 / portTICK_PERIOD_MS));
		// printf("Struct Received on Queue:\nCold Junction: %0.2f\nTemperature: %0.2f\n", result.coldjunction_f, result.thermocouple_f);
		// // printf("Fault: %d\n", max31856_result.fault);
		// printf("Cold Junction: %0.2f\n", result.coldjunction_c);
		// printf("Thermocouple: %0.2f\n", result.thermocouple_c);
		if (result.thermocouple_c != lastTemp || result.coldjunction_c != lastColdTemp)
		{
			lastTemp = result.thermocouple_c;
			lastColdTemp = result.coldjunction_c;

			instance->myLastResult = result;
			instance->myTempUi->SetCurrentTemp(result.thermocouple_c);
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void TempController::initPID()
{
	// Initialize PID controller
	myAutoPIDRelay = new AutoPIDRelay(&myLastCurrentTemp, &myLastSetTemp, &myRelayState, 2000.0, 30.0, 50.0, 0.1);
	myAutoPIDRelay->setBangBang(SSR_REDUCED_PWM_UNDER);
	myAutoPIDRelay->setTimeStep(1000);
	myAutoPIDRelay->setOutputRange(SSR_OFF_PWM, SSR_FULL_PWM);

	// Initialize SSR
	initSSR();
}

void TempController::initSSR()
{
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
	ESP_LOGI(TCTAG, "GPIO 22 configured as output for SSR control");

	// Ensure the heater is initially off
	setSSRDutyCycle(SSR_OFF_PWM);
}

void TempController::setSSRDutyCycle(int duty)
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
}
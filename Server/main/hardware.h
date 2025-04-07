#pragma once

#include <cstdint>
#include <soc/gpio_num.h>

static constexpr float MAX_TEMP = 1350.0f; // maximum temperature for K type thermocouple, higher than this may be an error also
static constexpr float MIN_TEMP = 5.0f;	   // there may be a thermocouple error below this

static constexpr gpio_num_t HEATER_SSR_PIN = GPIO_NUM_22;
static constexpr gpio_num_t EMERGENCY_RELAY_PIN = GPIO_NUM_17;
static constexpr bool EMERGENCY_RELAY_ON = false; // emergency relay is shutting off power when the pin is low
static constexpr gpio_num_t DOOR_SWITCH_PIN = GPIO_NUM_5;
static constexpr gpio_num_t ENABLE_SWITCH_PIN = GPIO_NUM_16;
static constexpr gpio_num_t CURRENT_SENSOR_PIN = GPIO_NUM_34;

static constexpr gpio_num_t MAX31856_SPI3_CLK = GPIO_NUM_23;
static constexpr gpio_num_t MAX31856_SPI3_MOSI = GPIO_NUM_19;
static constexpr gpio_num_t MAX31856_SPI3_MISO = GPIO_NUM_18;
static constexpr gpio_num_t MAX31856_SPI3_CS = GPIO_NUM_27;

static constexpr uint32_t DEBOUNCE_DELAY_MS = 50; // debounce delay in milliseconds

#define MAX31856_SPI SPI3_HOST

// SPI bus access synchronization
#define SPI3_BUS_TIMEOUT_MS 1000
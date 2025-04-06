#pragma once

#include <soc/gpio_num.h>

static constexpr gpio_num_t HEATER_SSR_PIN = GPIO_NUM_22;
static constexpr gpio_num_t EMERGENCY_RELAY_PIN = GPIO_NUM_17;
static constexpr gpio_num_t DOOR_SWITCH_PIN = GPIO_NUM_5;
static constexpr gpio_num_t CURRENT_SENSOR_PIN = GPIO_NUM_34;

static constexpr gpio_num_t MAX31856_SPI3_CLK = GPIO_NUM_23;
static constexpr gpio_num_t MAX31856_SPI3_MOSI = GPIO_NUM_19;
static constexpr gpio_num_t MAX31856_SPI3_MISO = GPIO_NUM_18;
static constexpr gpio_num_t MAX31856_SPI3_CS = GPIO_NUM_27;

#define MAX31856_SPI SPI3_HOST

// SPI bus access synchronization
#define SPI3_BUS_TIMEOUT_MS 1000
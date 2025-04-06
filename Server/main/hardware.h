#pragma once

#define HEATER_SSR (gpio_num_t) GPIO_NUM_16
#define EMERGENCY_RELAY (gpio_num_t) GPIO_NUM_17
#define DOOR_SWITCH (gpio_num_t) GPIO_NUM_5
#define CURRENT_SENSOR (gpio_num_t) GPIO_NUM_34

#define MAX31856_SPI SPI3_HOST
#define MAX31856_SPI3_CLK (gpio_num_t) GPIO_NUM_23
#define MAX31856_SPI3_MOSI (gpio_num_t) GPIO_NUM_19
#define MAX31856_SPI3_MISO (gpio_num_t) GPIO_NUM_18
#define MAX31856_SPI3_CS (gpio_num_t) GPIO_NUM_27
#define HEATER_SSR_PIN (gpio_num_t) GPIO_NUM_22

// SPI bus access synchronization
#define SPI3_BUS_TIMEOUT_MS 1000
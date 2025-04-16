#pragma once

#include "driver/uart.h"
#include "pl_uart_types.h"
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

static constexpr uart_port_t MODBUS_UART_PORT = UART_NUM_1;
// static constexpr uart_port_t MODBUS_UART_PORT = UART_NUM_0;
static constexpr gpio_num_t MODBUS_TX = GPIO_NUM_13;
static constexpr gpio_num_t MODBUS_RX = GPIO_NUM_14;

static constexpr int LINE_FREQ = 60;
static constexpr float HEATING_BANG_BANG_WINDOW = 50;		 // if the temp is outside of this window of the target temp, it will switch from PID to bang-bang to rapidly heat/cool to that point
static constexpr float MAX_HEATING_RATE_PER_SECOND = 100.0f; // reduce this if your element is too powerful and you must prevent it from applying full speed heating. Defaults to a maximum of 10 degrees per second. Ideally, it should be larger than your bang bang window.
static constexpr float STARTUP_HEATING_RATE_PER_SECOND = 0.5f;
static constexpr float STARTUP_HEATING_RATE_UNDER_TEMP = 500.0f; // below this temperature, the startup heating rate is 0.5 degrees per second to slowly warm up the crucible
static constexpr float HEATING_RATE_TASK_PERIOD_MS = 1000.0f;	 // how often to calculate if we need to set the next internal target according to our heating rate schedule. For furnaces with a large mass, this should be multiple seconds so that the PID can accelerate properly when the target temp increments.

#define SIMULATED_TEMP_DEVICE 1

#if SIMULATED_TEMP_DEVICE
static constexpr float HEAT_RATE_PER_SECOND = 1.5f;
static constexpr float COOLING_RATE_PER_SECOND = 0.7f;
static constexpr float AMBIENT_TEMP = 25.0f;
#endif

#define MAX31856_SPI SPI3_HOST

// SPI bus access synchronization
#define SPI3_BUS_TIMEOUT_MS 1000
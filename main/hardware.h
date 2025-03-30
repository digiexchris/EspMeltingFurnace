#pragma once

#define LCD_H_RES 240
#define LCD_V_RES 320
#define LCD_BITS_PIXEL 16
#define LCD_BUF_LINES 30
#define LCD_DOUBLE_BUFFER 1
#define LCD_DRAWBUF_SIZE (LCD_H_RES * LCD_BUF_LINES)
#define LCD_MIRROR_X (true)
#define LCD_MIRROR_Y (false)

#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_CMD_BITS (8)
#define LCD_PARAM_BITS (8)
#define LCD_SPI_HOST SPI2_HOST
#define LCD_SPI_CLK (gpio_num_t) GPIO_NUM_14
#define LCD_SPI_MOSI (gpio_num_t) GPIO_NUM_13
#define LCD_SPI_MISO (gpio_num_t) GPIO_NUM_12
#define LCD_DC (gpio_num_t) GPIO_NUM_2
#define LCD_CS (gpio_num_t) GPIO_NUM_15
#define LCD_RESET (gpio_num_t) GPIO_NUM_4
#define LCD_BUSY (gpio_num_t) GPIO_NUM_NC

#define LCD_BACKLIGHT (gpio_num_t) GPIO_NUM_21
#define LCD_BACKLIGHT_LEDC_CH (1)

#define TOUCH_X_RES_MIN 0
#define TOUCH_X_RES_MAX 240
#define TOUCH_Y_RES_MIN 0
#define TOUCH_Y_RES_MAX 320

#define TOUCH_CLOCK_HZ ESP_LCD_TOUCH_SPI_CLOCK_HZ
#define TOUCH_SPI SPI3_HOST
#define TOUCH_SPI_CLK (gpio_num_t) GPIO_NUM_25
#define TOUCH_SPI_MOSI (gpio_num_t) GPIO_NUM_32
#define TOUCH_SPI_MISO (gpio_num_t) GPIO_NUM_39
#define TOUCH_CS (gpio_num_t) GPIO_NUM_33
#define TOUCH_DC (gpio_num_t) GPIO_NUM_NC
#define TOUCH_RST (gpio_num_t) GPIO_NUM_NC
#define TOUCH_IRQ (gpio_num_t) GPIO_NUM_36

#define SPK_CONNECTOR_PIN (gpio_num_t) GPIO_NUM_26

// RGB LED pins
// #define RGB_LED_R (gpio_num_t) GPIO_NUM_4 //REUSED AS LCD RESET!!!
#define RGB_LED_G (gpio_num_t) GPIO_NUM_16
#define RGB_LED_B (gpio_num_t) GPIO_NUM_17

// CDS
#define CDS_SENSOR_PIN (gpio_num_t) GPIO_NUM_34

// TF
#define TF_SPI3_CS (gpio_num_t) GPIO_NUM_5 // pulled up to 3v3
#define TF_SPI3_MOSI (gpio_num_t) GPIO_NUM_23
#define TF_SPI3_MISO (gpio_num_t) GPIO_NUM_19
#define TF_SPI3_CLK (gpio_num_t) GPIO_NUM_18

// on left 4 pin header. CANNOT USE 21 on that header! It's the LCD backlight
// on left header, 35 and 22 are available
// note
// Is input-only (34-39 are all input-only), 37, 38 are available inputs
// Is connected to internal flash (6-11)
#define I2C_SDA (gpio_num_t) GPIO_NUM_16
#define I2C_SCL (gpio_num_t) GPIO_NUM_17
#define I2C_MASTER_NUM I2C_NUM_0	/*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ 100000	/*!< I2C master clock frequency */

#define MAX31856_SPI3_CS (gpio_num_t) GPIO_NUM_27
#define HEATER_SSR_PIN (gpio_num_t) GPIO_NUM_22

enum MCP23017_PINS
{
	PA0 = 0,
	PA1 = 1,
	PA2 = 2,
	PA3 = 3,
	PA4 = 4,
	PA5 = 5,
	PA6 = 6,
	PA7 = 7
};

#define MCP23017_I2C_ADDR 0x20
#define MCP23017_I2C_SDA (gpio_num_t) GPIO_NUM_16
#define MCP23017_I2C_SCL (gpio_num_t) GPIO_NUM_17
#define MCP23017_I2C_INT0 (gpio_num_t) GPIO_NUM_35
#define MCP23017_I2C_INT1 (gpio_num_t) GPIO_NUM_5
#define MCP23017_I2C_ADDR 0x27
#define MCP23017_SW1 (int)0
#define MCP23017_ENC_SW (int)1
#define MCP23017_ENC_A (int)2
#define MCP23017_ENC_B (int)3

// SPI bus access synchronization
#define SPI3_BUS_TIMEOUT_MS 1000
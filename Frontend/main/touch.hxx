// #pragma once

// #include "SPIBus.hxx"
// #include <esp_err.h>
// #include <esp_lcd_touch.h>

// class Touch
// {
// public:
// 	Touch();

// private:
// 	static uint16_t map(uint16_t n, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
// 	static void processCoordinates(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
// 	esp_lcd_touch_handle_t myTouchHandle; // aka tp
// 	esp_lcd_panel_io_handle_t myTouchIoHandle;
// 	SPIBusManager *mySpiBusManager;
// }
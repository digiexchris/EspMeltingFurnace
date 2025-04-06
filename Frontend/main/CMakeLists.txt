# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# Register the main component
FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "../include" 
    REQUIRES esp_lcd driver max31856-espidf esp_lcd_touch_xpt2046 lvgl i2c_bus
)

# FTXUI ESP-IDF Component - Extract only needed sources

# Clone FTXUI if needed
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ftxui/CMakeLists.txt")
    execute_process(
        COMMAND git clone --depth=1 --branch v6.0.2 https://github.com/ArthurSonzogni/ftxui.git
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone FTXUI repository")
    endif()
endif()

# Get all source files for each FTXUI component
file(GLOB_RECURSE FTXUI_SCREEN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ftxui/src/ftxui/screen/*.cpp")
file(GLOB_RECURSE FTXUI_DOM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ftxui/src/ftxui/dom/*.cpp") 
file(GLOB_RECURSE FTXUI_COMPONENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ftxui/src/ftxui/component/*.cpp")

# Filter out test files from sources
list(FILTER FTXUI_SCREEN_SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")
list(FILTER FTXUI_DOM_SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")
list(FILTER FTXUI_COMPONENT_SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")

set(FTXUI_ESP32_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/ftxui_esp32_signal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ftxui_esp32_input.cpp"
)

# Register component with all FTXUI sources directly
idf_component_register(
    SRCS ${FTXUI_SCREEN_SOURCES} ${FTXUI_DOM_SOURCES} ${FTXUI_COMPONENT_SOURCES} ${FTXUI_ESP32_SOURCES}
    INCLUDE_DIRS "ftxui/include"
    REQUIRES pthread esp_event driver
)

# Add required compiler definitions for FTXUI
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    FTXUI_CONFIG_USE_STD_FUNCTION=1
    FTXUI_ENABLE_INSTALL=0
    FTXUI_BUILD_EXAMPLES=0
    FTXUI_BUILD_DOCS=0
    FTXUI_BUILD_TESTS=0
    FTXUI_USE_DETACHED_THREAD=0
)

# Add include directory for internal FTXUI headers 
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/ftxui/src"
)

# Set compiler options
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-missing-field-initializers
    -Wno-unused-parameter
)

# Create wrapper libraries for compatibility
add_library(ftxui_screen INTERFACE)
add_library(ftxui_dom INTERFACE)
add_library(ftxui_component INTERFACE)

# Link the wrapper libraries to our component
target_link_libraries(ftxui_screen INTERFACE ${COMPONENT_LIB})
target_link_libraries(ftxui_dom INTERFACE ${COMPONENT_LIB})
target_link_libraries(ftxui_component INTERFACE ${COMPONENT_LIB})

# Add aliases to match FTXUI's original target names
add_library(ftxui::screen ALIAS ftxui_screen)
add_library(ftxui::dom ALIAS ftxui_dom)
add_library(ftxui::component ALIAS ftxui_component)

message(STATUS "FTXUI component registered")
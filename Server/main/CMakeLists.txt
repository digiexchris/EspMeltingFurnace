# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# Register the main component
FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.* ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/*.*)

# Enable C++17 for the entire project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "../../lib"
    REQUIRES driver max31856-espidf ftxui
    PRIV_REQUIRES usb pthread
)

target_compile_options(${COMPONENT_LIB} PRIVATE -D_GLIBCXX_USE_C99=1)

# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# Register the main component
FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "../../lib"
    REQUIRES driver max31856-espidf
)

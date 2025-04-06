cmake_minimum_required(VERSION 3.16.0)

# Skip Git operations during the build
# set(SKIP_MAIN_SRCS_COMMANDS 1)
# set(IDF_PROJECT_EXECUTABLE EspMeltingFurnace)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(EspMeltingFurnace)

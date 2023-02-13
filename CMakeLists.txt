cmake_minimum_required(VERSION 3.12)

set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(clock)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
add_executable(clock main.c console.c command.c ds3231.c)

target_link_libraries(clock pico_stdlib pico_multicore hardware_i2c)
pico_add_extra_outputs(clock)

pico_enable_stdio_uart(clock 1)
pico_enable_stdio_usb(clock 1)

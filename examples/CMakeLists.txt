cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(fhdm-ad9833-pico-examples)

pico_sdk_init()

add_subdirectory(../lib build)

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
    $<$<CONFIG:Debug>:PARAM_ASSERTIONS_ENABLE_ALL=1>
  )

######################################
#
# example-simple
#
# Generate a signal using the library default values
# Frequency : 100Hz
# Phase : 0
# FSYNC : GPIO 5
# SCLK  : GPIO 6
# SDATA : GPIO 7
#

add_executable(example-simple
    example-simple.c
)

target_link_libraries(example-simple 
    fhdm-ad9833-pico
    pico_stdlib 
)

# enable usb output, disable uart output
pico_enable_stdio_usb(example-simple 0)
pico_enable_stdio_uart(example-simple 1)

target_compile_options(fhdm-ad9833-pico INTERFACE -Wall -Werror -Wno-error=unused-variable -Wno-error=unused-function)
target_compile_options(example-simple PRIVATE -Wall -Werror -Wno-error=unused-variable -Wno-error=unused-function)

pico_set_binary_type(example-simple no_flash)
pico_add_extra_outputs(example-simple)


######################################
#
# example-extra
#
# This example lets you change the output by connecting particular gpios to ground
# See the irq callback for details
#

add_executable(example-extra
    example-extra.c
)

target_link_libraries(example-extra 
    fhdm-ad9833-pico
    pico_stdlib 
)

# enable usb output, disable uart output
pico_enable_stdio_usb(example-extra 0)
pico_enable_stdio_uart(example-extra 1)

target_compile_options(fhdm-ad9833-pico INTERFACE -Wall -Werror -Wno-error=unused-variable -Wno-error=unused-function)
target_compile_options(example-extra PRIVATE -Wall -Werror -Wno-error=unused-variable -Wno-error=unused-function)

pico_set_binary_type(example-extra no_flash)
pico_add_extra_outputs(example-extra)

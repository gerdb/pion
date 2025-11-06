# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
set (PICO_EXTRAS_PATH ${USERHOME}/.pico-sdk/pico-extras)
include(pico_extras_import.cmake)

project(pion C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(pion
        main.c
        copro.c
        PicoTM1637.c
        )

# Generate PIO header
pico_generate_pio_header(pion
    ${CMAKE_CURRENT_LIST_DIR}/PicoTM1637.pio
)

# pull in common dependencies and additional pwm hardware support
target_link_libraries(pion
        pico_stdlib
        hardware_adc
        pico_multicore
        hardware_pio
        )

target_compile_definitions(pion PRIVATE
        )

pico_enable_stdio_usb(pion 1)
pico_enable_stdio_uart(pion 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(pion)

# add url via pico_set_program_url

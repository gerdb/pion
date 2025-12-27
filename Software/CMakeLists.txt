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

# add a compilation subdirectory
add_subdirectory(lib/Config)
add_subdirectory(lib/e-Paper)
add_subdirectory(lib/Fonts)
add_subdirectory(lib/GUI)
add_subdirectory(pics)

# add a header directory
include_directories(./lib/Config)
include_directories(./lib/e-Paper)
include_directories(./lib/GUI)
include_directories(./pics)


add_executable(pion
        main.c
        copro.c
        )


# pull in common dependencies and additional pwm hardware support
target_link_libraries(pion
        pico_stdlib
        hardware_adc
        hardware_pwm
        pico_multicore
        hardware_pio
        ePaper
        GUI
        Fonts
        Config
        pics
        hardware_spi
        )

target_compile_definitions(pion PRIVATE
        DEBUG
        )

pico_enable_stdio_usb(pion 1)
pico_enable_stdio_uart(pion 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(pion)

# add url via pico_set_program_url

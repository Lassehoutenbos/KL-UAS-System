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
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# -- FreeRTOS-Kernel (Raspberry Pi fork) via FetchContent --
# The RP2350_ARM_NTZ port is provided by the raspberrypi/FreeRTOS-Kernel fork.
# This fork is integrated by adding the *port* directory as a subdirectory
# (NOT the top-level FreeRTOS-Kernel CMakeLists, which has no knowledge of
# the GCC_RP2350_ARM_NTZ port). The port hooks into pico_sdk_init() via
# PICO_SDK_POST_LIST_FILES and creates FreeRTOS-Kernel + FreeRTOS-Kernel-Heap*
# INTERFACE targets.
include(FetchContent)
FetchContent_Declare(
    freertos_kernel
    GIT_REPOSITORY https://github.com/raspberrypi/FreeRTOS-Kernel.git
    GIT_TAG        main
)
FetchContent_GetProperties(freertos_kernel)
if(NOT freertos_kernel_POPULATED)
    FetchContent_Populate(freertos_kernel)
endif()
set(FREERTOS_KERNEL_PATH ${freertos_kernel_SOURCE_DIR})

# freertos_config must be defined before the port subdirectory is added.
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}   # FreeRTOSConfig.h lives at project root
)
target_compile_definitions(freertos_config INTERFACE
    projCOVERAGE_TEST=0
)

# Pull in the RP2350 ARM non-TrustZone port. This must come BEFORE
# pico_sdk_init() so it can register its library.cmake via
# PICO_SDK_POST_LIST_FILES.
add_subdirectory(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ
                 ${CMAKE_BINARY_DIR}/freertos_kernel_port)

project(GCS C CXX ASM)

# Initialise the Raspberry Pi Pico SDK (this triggers FreeRTOS library.cmake)
pico_sdk_init()

# -- Source files --
add_executable(GCS
    src/main.c
    src/system_state.c
    src/protocol.c
    src/usb_cdc.c
    src/usb_descriptors.c
    src/analog.c
    src/digital_io.c
    src/led_sk6812.c
    src/led_ws2811.c
    src/screen_st7735.c
    src/screen_display.c
    src/veml7700.c
    src/rs485.c
)

pico_set_program_name(GCS "GCS")
pico_set_program_version(GCS "1.0")

# UART stdio on, USB stdio OFF — TinyUSB owns the USB hardware
pico_enable_stdio_uart(GCS 1)
pico_enable_stdio_usb(GCS 0)

# -- PIO header generation --
pico_generate_pio_header(GCS ${CMAKE_CURRENT_LIST_DIR}/src/sk6812.pio)
pico_generate_pio_header(GCS ${CMAKE_CURRENT_LIST_DIR}/src/ws2811.pio)

# -- Include directories --
target_include_directories(GCS PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}       # FreeRTOSConfig.h, tusb_config.h
    ${CMAKE_CURRENT_LIST_DIR}/src   # pins.h, protocol.h, etc.
)

# -- Link libraries --
target_link_libraries(GCS
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_pio
    hardware_dma
    hardware_gpio
    hardware_clocks
    hardware_uart
    hardware_pwm
    pico_unique_id
    tinyusb_device
    tinyusb_board
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)

pico_add_extra_outputs(GCS)

# ------------------------------------------------------------------ #
# TFT display self-test (flash instead of GCS.uf2 to test display)   #
# ------------------------------------------------------------------ #
add_executable(GCS_TFT_Test
    src/test_tft.c
    src/screen_st7735.c
)

pico_set_program_name(GCS_TFT_Test "GCS_TFT_Test")
pico_set_program_version(GCS_TFT_Test "1.0")

pico_enable_stdio_uart(GCS_TFT_Test 0)
pico_enable_stdio_usb(GCS_TFT_Test 0)

target_include_directories(GCS_TFT_Test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(GCS_TFT_Test
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_gpio
    hardware_pwm
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)

pico_add_extra_outputs(GCS_TFT_Test)

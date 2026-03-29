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
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(GCS C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# -- FreeRTOS-Kernel via FetchContent --
# Must be after pico_sdk_init() so the ARM toolchain is set up before
# FreeRTOS-Kernel processes its own CMakeLists.
include(FetchContent)
FetchContent_Declare(
    freertos_kernel
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG        V11.1.0
)
# freertos_config must be defined before FetchContent_MakeAvailable.
# Link hardware_sync and hardware_exception so the RP2040 port can find
# hardware/sync.h and hardware/exception.h from the Pico SDK.
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}   # FreeRTOSConfig.h lives at project root
)
target_compile_definitions(freertos_config INTERFACE
    projCOVERAGE_TEST=0
)
target_link_libraries(freertos_config INTERFACE
    hardware_sync
    hardware_exception
    pico_multicore      # RP2040 port uses sio_hw inter-core FIFO
)
set(FREERTOS_HEAP "4" CACHE STRING "FreeRTOS heap implementation")
set(FREERTOS_PORT "GCC_RP2040" CACHE STRING "FreeRTOS port for RP2040 Cortex-M0+")
FetchContent_MakeAvailable(freertos_kernel)

# Capture the FreeRTOS source path for explicit include propagation
FetchContent_GetProperties(freertos_kernel SOURCE_DIR FREERTOS_KERNEL_PATH)

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
    # FreeRTOS headers (explicit — PUBLIC propagation from FetchContent is unreliable)
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include
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
    pico_unique_id
    tinyusb_device
    tinyusb_board
    freertos_kernel
)

pico_add_extra_outputs(GCS)

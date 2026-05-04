# ESP32 backend for the peripheral framework.
#
# ESP-IDF has its own component-based build system on top of CMake. The
# right way to consume the framework from ESP-IDF is to register it as
# a component, not to call add_executable directly. This file provides
# that bridge so apps can keep the same `add_peripheral(... MCU esp32 ...)`
# call as the other MCUs.
#
# Build prerequisites:
#   - ESP-IDF v5.x set up (idf.py exports IDF_PATH)
#   - The app directory must contain an idf.py-compatible project
#     skeleton (sdkconfig defaults, CMakeLists.txt that does
#     `include($ENV{IDF_PATH}/tools/cmake/project.cmake)`).
#
# When TARGET_MCU=esp32, the app's own CMakeLists is expected to be a
# regular IDF project. add_peripheral() then declares an IDF component
# from the framework + HAL sources and links it into the main app.

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR
        "esp32.cmake: IDF_PATH not set. Source ESP-IDF's export.sh first.")
endif()

function(_peripheral_esp32 NAME SOURCES)
    # Register a private component carrying the framework core + ESP HAL.
    idf_component_register(
        SRCS
            ${PERIPH_CORE_SOURCES}
            ${PERIPH_FRAMEWORK_DIR}/hal/hal_esp32.c
            ${SOURCES}
        INCLUDE_DIRS
            ${PERIPH_CORE_INCLUDES}
        REQUIRES
            driver
            esp_timer
    )
    # The IDF flow produces NAME.bin/elf via idf.py build — no extra
    # post-build step needed here.
endfunction()

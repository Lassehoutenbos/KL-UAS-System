cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED TARGET_MCU)
    set(TARGET_MCU stm32f1 CACHE STRING "Target MCU (stm32f1 | rp2040 | esp32)")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../Framework/cmake/framework.cmake)

project(lightbar C ASM)

set(BOARD_FILE ${CMAKE_CURRENT_LIST_DIR}/src/board_${TARGET_MCU}.c)
if(NOT EXISTS ${BOARD_FILE})
    message(FATAL_ERROR
        "LightBar has no board implementation for MCU '${TARGET_MCU}'. "
        "Expected: ${BOARD_FILE}")
endif()

add_peripheral(
    NAME    lightbar
    MCU     ${TARGET_MCU}
    SOURCES src/main.c ${BOARD_FILE}
)
target_include_directories(lightbar PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

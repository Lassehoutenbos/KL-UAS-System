# RP2040 backend for the peripheral framework.
#
# Pulls the Raspberry Pi Pico SDK (via PICO_SDK_PATH or pico-sdk install),
# wires the framework core + RP2040 HAL, and produces a UF2 next to the
# ELF.
#
# pico_sdk_init() must be called once per project. Apps may set
# PICO_BOARD before including framework.cmake to target a specific board
# (default = pico).

# pico_sdk_import.cmake sets up the toolchain and must run BEFORE
# project(), so it lives at include time. pico_sdk_init() inspects
# project state and must run AFTER project(), so it's deferred into
# the per-target function below.
if(NOT DEFINED PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()
if(NOT PICO_SDK_PATH)
    message(FATAL_ERROR
        "rp2040.cmake: PICO_SDK_PATH not set. "
        "Install the Pico SDK and set the env var, or pass "
        "-DPICO_SDK_PATH=/path/to/pico-sdk on the cmake command line.")
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico)
endif()

function(_peripheral_rp2040 NAME SOURCES)
    if(NOT COMMAND pico_add_extra_outputs)
        pico_sdk_init()
    endif()
    add_executable(${NAME}
        ${SOURCES}
        ${PERIPH_CORE_SOURCES}
        ${PERIPH_FRAMEWORK_DIR}/hal/hal_rp2040.c
    )
    target_include_directories(${NAME} PRIVATE ${PERIPH_CORE_INCLUDES})
    target_link_libraries(${NAME}
        pico_stdlib
        hardware_uart
        hardware_gpio
        hardware_pwm
    )
    pico_add_extra_outputs(${NAME})   # produces NAME.uf2
endfunction()

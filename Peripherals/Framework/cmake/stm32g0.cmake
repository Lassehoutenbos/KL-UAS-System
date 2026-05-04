# STM32G031 backend for the peripheral framework.
#
# Pulls CMSIS Core (CMSIS_5) and CMSIS-Device-G0 via FetchContent so the
# repo doesn't vendor headers. Sets the toolchain to arm-none-eabi-gcc
# and uses ST's stock startup .s + linker script from CMSIS-Device-G0.
#
# Build prerequisites: arm-none-eabi-gcc on PATH (e.g. apt-get
# install gcc-arm-none-eabi).

if(NOT CMAKE_C_COMPILER MATCHES "arm-none-eabi-gcc")
    set(CMAKE_SYSTEM_NAME      Generic)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(CMAKE_C_COMPILER       arm-none-eabi-gcc)
    set(CMAKE_ASM_COMPILER     arm-none-eabi-gcc)
    set(CMAKE_OBJCOPY          arm-none-eabi-objcopy)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()

include(FetchContent)
FetchContent_Declare(cmsis_core
    GIT_REPOSITORY https://github.com/ARM-software/CMSIS_5.git
    GIT_TAG        5.9.0
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(cmsis_device_g0
    GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_g0.git
    GIT_TAG        v1.4.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(cmsis_core cmsis_device_g0)

set(STM32_CFLAGS
    -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft
    -ffunction-sections -fdata-sections -Os -g
)
set(STM32_LDFLAGS
    -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft
    -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs
)

set(STM32G0_DEVICE_DIR ${cmsis_device_g0_SOURCE_DIR})
set(STM32G0_STARTUP    ${STM32G0_DEVICE_DIR}/Source/Templates/gcc/startup_stm32g031xx.s)
set(STM32G0_LINKER     ${STM32G0_DEVICE_DIR}/Source/Templates/gcc/linker/STM32G031K8Tx_FLASH.ld)
set(STM32G0_SYSTEM     ${STM32G0_DEVICE_DIR}/Source/Templates/system_stm32g0xx.c)

function(_peripheral_stm32g0 NAME SOURCES)
    add_executable(${NAME}
        ${SOURCES}
        ${PERIPH_CORE_SOURCES}
        ${PERIPH_FRAMEWORK_DIR}/hal/hal_stm32g0.c
        ${STM32G0_STARTUP}
        ${STM32G0_SYSTEM}
    )
    target_compile_definitions(${NAME} PRIVATE STM32G031xx)
    target_compile_options(${NAME} PRIVATE ${STM32_CFLAGS})
    target_include_directories(${NAME} PRIVATE
        ${PERIPH_CORE_INCLUDES}
        ${cmsis_core_SOURCE_DIR}/CMSIS/Core/Include
        ${STM32G0_DEVICE_DIR}/Include
    )
    target_link_options(${NAME} PRIVATE
        ${STM32_LDFLAGS}
        -T${STM32G0_LINKER}
        -Wl,-Map=${NAME}.map
    )
    set_target_properties(${NAME} PROPERTIES SUFFIX ".elf")

    add_custom_command(TARGET ${NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${NAME}> ${NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${NAME}> ${NAME}.hex
        COMMENT "Generating ${NAME}.bin / ${NAME}.hex"
    )
endfunction()

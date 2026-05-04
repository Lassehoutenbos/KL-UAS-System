# STM32F103C8T6 backend for the peripheral framework.
#
# Pulls CMSIS Core (CMSIS_5) and CMSIS-Device-F1 via FetchContent so the
# repo doesn't vendor headers. Sets the toolchain to arm-none-eabi-gcc
# and uses ST's stock startup .s + linker script from CMSIS-Device-F1.
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
FetchContent_Declare(cmsis_device_f1
    GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_f1.git
    GIT_TAG        v4.3.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(cmsis_core cmsis_device_f1)

set(STM32_CFLAGS
    -mcpu=cortex-m3 -mthumb
    -ffunction-sections -fdata-sections -Os -g
)
set(STM32_LDFLAGS
    -mcpu=cortex-m3 -mthumb
    -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs
    # arm-none-eabi-gcc 14.x newlib-nano references _init/_fini from
    # __libc_init_array but no longer ships them. We have no C++ static
    # ctors here, so stub them to 0.
    -Wl,--defsym=_init=0 -Wl,--defsym=_fini=0
)

set(STM32F1_DEVICE_DIR ${cmsis_device_f1_SOURCE_DIR})
set(STM32F1_STARTUP    ${STM32F1_DEVICE_DIR}/Source/Templates/gcc/startup_stm32f103xb.s)
set(STM32F1_LINKER     ${STM32F1_DEVICE_DIR}/Source/Templates/gcc/linker/STM32F103XB_FLASH.ld)
set(STM32F1_SYSTEM     ${STM32F1_DEVICE_DIR}/Source/Templates/system_stm32f1xx.c)

function(_peripheral_stm32f1 NAME SOURCES)
    add_executable(${NAME}
        ${SOURCES}
        ${PERIPH_CORE_SOURCES}
        ${PERIPH_FRAMEWORK_DIR}/hal/hal_stm32f1.c
        ${STM32F1_STARTUP}
        ${STM32F1_SYSTEM}
    )
    target_compile_definitions(${NAME} PRIVATE STM32F103xB)
    target_compile_options(${NAME} PRIVATE ${STM32_CFLAGS})
    target_include_directories(${NAME} PRIVATE
        ${PERIPH_CORE_INCLUDES}
        ${cmsis_core_SOURCE_DIR}/CMSIS/Core/Include
        ${STM32F1_DEVICE_DIR}/Include
    )
    target_link_options(${NAME} PRIVATE
        ${STM32_LDFLAGS}
        -T${STM32F1_LINKER}
        -Wl,-Map=${NAME}.map
    )
    set_target_properties(${NAME} PROPERTIES SUFFIX ".elf")

    add_custom_command(TARGET ${NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${NAME}> ${NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${NAME}> ${NAME}.hex
        COMMENT "Generating ${NAME}.bin / ${NAME}.hex"
    )
endfunction()

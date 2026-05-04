# RS-485 peripheral slave framework — top-level CMake entry point.
#
# Apps include this file and call:
#   add_peripheral(NAME <app> MCU <stm32f1|rp2040|esp32> SOURCES <files…>)
#
# The MCU dispatcher pulls in the per-MCU toolchain + SDK and adds the
# framework's portable core + the matching HAL.

set(PERIPH_FRAMEWORK_DIR ${CMAKE_CURRENT_LIST_DIR}/.. CACHE INTERNAL "")
set(PERIPH_CMAKE_DIR     ${CMAKE_CURRENT_LIST_DIR}    CACHE INTERNAL "")

set(PERIPH_CORE_SOURCES
    ${PERIPH_FRAMEWORK_DIR}/core/crc8.c
    ${PERIPH_FRAMEWORK_DIR}/core/rs485_slave.c
    CACHE INTERNAL ""
)
set(PERIPH_CORE_INCLUDES
    ${PERIPH_FRAMEWORK_DIR}/core
    ${PERIPH_FRAMEWORK_DIR}/hal
    CACHE INTERNAL ""
)

# The MCU backend must be loaded BEFORE project() so that toolchain
# variables (CMAKE_C_COMPILER etc.) are in place when CMake probes the
# compiler. App CMakeLists.txt include framework.cmake before project(),
# so dispatch on TARGET_MCU here.
if(NOT DEFINED TARGET_MCU)
    set(TARGET_MCU stm32f1 CACHE STRING "Target MCU (stm32f1 | rp2040 | esp32)")
endif()

if(TARGET_MCU STREQUAL "rp2040")
    include(${PERIPH_CMAKE_DIR}/rp2040.cmake)
elseif(TARGET_MCU STREQUAL "stm32f1")
    include(${PERIPH_CMAKE_DIR}/stm32f1.cmake)
elseif(TARGET_MCU STREQUAL "esp32")
    include(${PERIPH_CMAKE_DIR}/esp32.cmake)
else()
    message(FATAL_ERROR "framework.cmake: unknown TARGET_MCU '${TARGET_MCU}' "
                        "(expected rp2040 | stm32f1 | esp32)")
endif()

function(add_peripheral)
    cmake_parse_arguments(P "" "NAME;MCU" "SOURCES" ${ARGN})
    if(NOT P_NAME OR NOT P_MCU)
        message(FATAL_ERROR "add_peripheral: NAME and MCU are required")
    endif()
    if(NOT P_MCU STREQUAL TARGET_MCU)
        message(FATAL_ERROR
            "add_peripheral: MCU '${P_MCU}' does not match TARGET_MCU "
            "'${TARGET_MCU}' resolved at framework include time. "
            "Set TARGET_MCU before include(framework.cmake).")
    endif()
    cmake_language(CALL _peripheral_${P_MCU} ${P_NAME} "${P_SOURCES}")
endfunction()

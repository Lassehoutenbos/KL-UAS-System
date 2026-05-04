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

function(add_peripheral)
    cmake_parse_arguments(P "" "NAME;MCU" "SOURCES" ${ARGN})
    if(NOT P_NAME OR NOT P_MCU)
        message(FATAL_ERROR "add_peripheral: NAME and MCU are required")
    endif()

    if(P_MCU STREQUAL "rp2040")
        include(${PERIPH_CMAKE_DIR}/rp2040.cmake)
        _peripheral_rp2040(${P_NAME} "${P_SOURCES}")
    elseif(P_MCU STREQUAL "stm32f1")
        include(${PERIPH_CMAKE_DIR}/stm32f1.cmake)
        _peripheral_stm32f1(${P_NAME} "${P_SOURCES}")
    elseif(P_MCU STREQUAL "esp32")
        include(${PERIPH_CMAKE_DIR}/esp32.cmake)
        _peripheral_esp32(${P_NAME} "${P_SOURCES}")
    else()
        message(FATAL_ERROR "add_peripheral: unknown MCU '${P_MCU}' "
                            "(expected rp2040 | stm32f1 | esp32)")
    endif()
endfunction()

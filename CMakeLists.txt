cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

project(adas_mcu_dev C CXX ASM)

set(STM32_TARGET "stm32f1" CACHE STRING "STM32 platform target: stm32f1, stm32f4")
set_property(CACHE STM32_TARGET PROPERTY STRINGS stm32f1 stm32f4)
option(ADAS_USB_COMPOSITE "Enable STM32F4 TinyUSB HID + CDC composite device" OFF)
option(ADAS_USB_EXTRA_SERVICE_CDC "Expose an extra service/config CDC ACM port in USB composite mode" OFF)

if (ADAS_USB_EXTRA_SERVICE_CDC AND NOT ADAS_USB_COMPOSITE)
    message(FATAL_ERROR "ADAS_USB_EXTRA_SERVICE_CDC requires ADAS_USB_COMPOSITE=ON.")
endif()

if (ADAS_USB_COMPOSITE AND NOT STM32_TARGET STREQUAL "stm32f4")
    message(FATAL_ERROR "ADAS_USB_COMPOSITE is currently implemented only for STM32_TARGET=stm32f4.")
endif()

set(STM32_TARGET_FILE ${CMAKE_CURRENT_SOURCE_DIR}/platform/${STM32_TARGET}/target.cmake)
if (NOT EXISTS ${STM32_TARGET_FILE})
    message(FATAL_ERROR "Unsupported STM32_TARGET='${STM32_TARGET}'. Expected a folder under platform/.")
endif()
include(${STM32_TARGET_FILE})

enable_language(C CXX ASM)

add_compile_options(${STM32_CPU_PARAMETERS})
add_link_options(${STM32_CPU_PARAMETERS})

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(EXECUTABLE ${CMAKE_PROJECT_NAME})
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel)

file(GLOB_RECURSE APPLICATION_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set(FREERTOS_PORT "${STM32_FREERTOS_PORT}" CACHE STRING "" FORCE)

add_subdirectory(${FREERTOS_KERNEL_PATH} FreeRTOS-Kernel)

target_compile_options(freertos_kernel PRIVATE
    ${STM32_CPU_PARAMETERS}
    -Wall -Wextra -Wundef -Wshadow -Wredundant-decls
    -fno-common -ffunction-sections -fdata-sections -MD
    $<$<CONFIG:Debug>:-Og -g3 -ggdb>
    $<$<CONFIG:Release>:-Os -g0>)

add_executable(${EXECUTABLE}
    ${APPLICATION_SOURCES}
    ${STM32_PLATFORM_SOURCES}
    ${STM32_STD_LIB_SOURCES}
    ${STM32_STARTUP_SCRIPT})

if (ADAS_USB_COMPOSITE)
    set(TINYUSB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyusb)

    target_sources(${EXECUTABLE} PRIVATE
        ${TINYUSB_ROOT}/src/tusb.c
        ${TINYUSB_ROOT}/src/common/tusb_fifo.c
        ${TINYUSB_ROOT}/src/device/usbd.c
        ${TINYUSB_ROOT}/src/device/usbd_control.c
        ${TINYUSB_ROOT}/src/class/cdc/cdc_device.c
        ${TINYUSB_ROOT}/src/class/hid/hid_device.c
        ${TINYUSB_ROOT}/src/portable/synopsys/dwc2/dwc2_common.c
        ${TINYUSB_ROOT}/src/portable/synopsys/dwc2/dcd_dwc2.c
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f4/usb/usb_composite_device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f4/usb/usb_descriptors.c)

    target_include_directories(${EXECUTABLE} PRIVATE
        ${TINYUSB_ROOT}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f4/usb)
endif()

target_compile_definitions(${EXECUTABLE} PRIVATE
    ${STM32_DEVICE_DEFINE}
    ${STM32_DENSITY_DEFINE}
    USE_STDPERIPH_DRIVER
    HSE_VALUE=${STM32_HSE_VALUE}
    ADAS_USB_COMPOSITE=$<BOOL:${ADAS_USB_COMPOSITE}>
    ADAS_USB_EXTRA_SERVICE_CDC=$<BOOL:${ADAS_USB_EXTRA_SERVICE_CDC}>
    ADAS_USB_CDC_COUNT=$<IF:$<BOOL:${ADAS_USB_EXTRA_SERVICE_CDC}>,2,1>)

target_include_directories(${EXECUTABLE} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/${STM32_TARGET}
    ${STM32_STD_LIB_INCLUDE_DIRECTORIES})

target_compile_options(${EXECUTABLE} PRIVATE
    ${STM32_CPU_PARAMETERS}
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    $<$<COMPILE_LANGUAGE:CXX>:
        -Wno-volatile
        -Wno-register
        -Wsuggest-override>
    $<$<CONFIG:Debug>:-Og -g3 -ggdb>
    $<$<CONFIG:Release>:-Os -g0>)

target_link_options(${EXECUTABLE} PRIVATE
    -T${STM32_LINKER_SCRIPT}
    ${STM32_CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--no-warn-rwx-segments
    --specs=nosys.specs
    -Wl,--start-group
    -lc
    -lm
    -lstdc++
    -Wl,--end-group
    -Wl,--print-memory-usage)

target_link_libraries(${EXECUTABLE} PRIVATE freertos_kernel freertos_config)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXECUTABLE}>)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.bin)

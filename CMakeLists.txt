cmake_minimum_required(VERSION 3.20)
project(stm32_feature_component LANGUAGES C CXX)

set(STM32F4_DRIVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/STM32F4_Driver" CACHE PATH
    "Path to the STM32F4 standard peripheral driver checkout")
set(FREERTOS_KERNEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel" CACHE PATH
    "Path to the FreeRTOS kernel checkout")

file(GLOB FEATURE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(${PROJECT_NAME} STATIC ${FEATURE_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC c_std_11 cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32F407xx STM32F40_41xxx USE_STDPERIPH_DRIVER HSE_VALUE=8000000)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src"
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
    "${STM32F4_DRIVER_DIR}/STM32F4xx_StdPeriph_Driver/inc"
    "${STM32F4_DRIVER_DIR}/CMSIS/Device/ST/STM32F4xx/Include"
    "${STM32F4_DRIVER_DIR}/CMSIS/Include"
    "${STM32F4_DRIVER_DIR}/CMSIS/RTOS")

if(EXISTS "${FREERTOS_KERNEL_DIR}/include/FreeRTOS.h")
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${FREERTOS_KERNEL_DIR}/include"
        "${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM4F")
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    -Wall -Wextra -Wno-unused-parameter
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os>)

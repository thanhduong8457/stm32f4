cmake_minimum_required(VERSION 3.20)
project(stm32_pid_control LANGUAGES C)

file(GLOB FEATURE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(${PROJECT_NAME} STATIC ${FEATURE_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC c_std_11)
target_compile_definitions(${PROJECT_NAME} PRIVATE STM32F407xx USE_HAL_DRIVER)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include")
target_compile_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    -Wall -Wextra -Wno-unused-parameter
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os>)

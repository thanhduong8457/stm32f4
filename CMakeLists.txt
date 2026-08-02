cmake_minimum_required(VERSION 3.20)
project(stm32f4_template LANGUAGES C CXX ASM)

set(STM32F4_DRIVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/STM32F4_Driver" CACHE PATH
    "Path to the STM32F4 standard peripheral driver checkout")

set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB DRIVER_SOURCES CONFIGURE_DEPENDS
    "${STM32F4_DRIVER_DIR}/STM32F4xx_StdPeriph_Driver/src/*.c")

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${DRIVER_SOURCES}
    startup/STM32F4xx/STM32F407xx.s)
target_compile_features(${PROJECT_NAME} PRIVATE c_std_11 cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32F407xx STM32F40_41xxx USE_STDPERIPH_DRIVER HSE_VALUE=8000000)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${STM32F4_DRIVER_DIR}/STM32F4xx_StdPeriph_Driver/inc"
    "${STM32F4_DRIVER_DIR}/CMSIS/Device/ST/STM32F4xx/Include"
    "${STM32F4_DRIVER_DIR}/CMSIS/Include")
target_compile_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS} -Wall -Wextra -Wno-unused-parameter
    $<$<COMPILE_LANGUAGE:ASM>:-Wa,-I${CMAKE_CURRENT_SOURCE_DIR}/startup>
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os>)
target_link_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    -Wl,-L${CMAKE_CURRENT_SOURCE_DIR}/linker
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F4xx/STM32F407xG.ld
    --specs=nosys.specs
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin)

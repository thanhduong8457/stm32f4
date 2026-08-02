# Template CMakeLists.txt configuration for STM32 MCUS
cmake_minimum_required(VERSION 3.12)

# ##############################################################################
# Edit your MCU information up to the next border
set(HAVE_FLAG_SEARCH_PATHS_FIRST 0)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(detal_robot_control C CXX ASM)
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(MCU_FAMILY STM32F4xx)
set(MCU_MODEL STM32F407xx)

set(CPU_PARAMETERS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(OPENCM3_COMPILER_FLAGS -DSTM32F4)
set(OPENCM3_MCU_LIB opencm3_stm32f4)

set(RTOS_PATH ${PROJECT_DIR}/FreeRTOS-Kernel)
set(RTOS_PORTABLE ARM_CM4F)

enable_language(C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(STARTUP_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/startup/STM32F407xx.s)
set(MCU_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F407xG.ld)

# ##############################################################################
set(EXECUTABLE ${CMAKE_PROJECT_NAME})
enable_language(C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Headers
set(CUBEMX_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32F4_Driver/${MCU_FAMILY}_StdPeriph_Driver/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32F4_Driver/${MCU_FAMILY}_StdPeriph_Driver/src
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32F4_Driver/CMSIS/Device/ST/${MCU_FAMILY}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32F4_Driver/CMSIS/Include
)

set(PROJECT_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_DIR}
)

# Sources
file(GLOB_RECURSE STM32CUBEMX_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32F4_Driver/${MCU_FAMILY}_StdPeriph_Driver/src/*.c
)

file(GLOB_RECURSE PROJECT_SOURCES FOLLOW_SYMLINKS
    ${PROJECT_DIR}/src/*.cpp
    ${PROJECT_DIR}/src/*.c
)

FILE(GLOB FreeRTOS_src ${RTOS_PATH}/*.c)

add_library(FreeRTOS STATIC
    ${FreeRTOS_src}
    ${RTOS_PATH}/portable/GCC/${RTOS_PORTABLE}/port.c
    ${RTOS_PATH}/portable/MemMang/heap_4.c
)

target_include_directories(FreeRTOS PUBLIC
    ${RTOS_PATH}/include
    ${RTOS_PATH}/portable/GCC/${RTOS_PORTABLE}/
    ${PROJECT_DIR}/src
)

target_compile_options(FreeRTOS PRIVATE
    ${CPU_PARAMETERS}
    ${OPENCM3_COMPILER_FLAGS}
    -Wall -Wextra -Wundef -Wshadow -Wredundant-decls
    -fno-common -ffunction-sections -fdata-sections -MD
    $<$<CONFIG:Debug>:-Og -g3 -ggdb>
    $<$<CONFIG:Release>:-Og -g0>
)

# Executable files
add_executable(${EXECUTABLE}
    ${STM32CUBEMX_SOURCES} 
    ${PROJECT_SOURCES}
    ${STARTUP_SCRIPT}
)

# Embedded macros(defines)
target_compile_definitions(${EXECUTABLE} PRIVATE
    ${MCU_MODEL}
    USE_STDPERIPH_DRIVER
    STM32F40_41xxx
    HSE_VALUE=8000000
)

# Add header directories (AFTER add_executable !!)
target_include_directories(${EXECUTABLE} PRIVATE
    ${CUBEMX_INCLUDE_DIRECTORIES}
    ${PROJECT_INCLUDE_DIRECTORIES}
    ${RTOS_PATH}/include
)

# Compiler and linker options
target_compile_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    $<$<COMPILE_LANGUAGE:CXX>:
        -Wno-volatile
        -Wold-style-cast
        -Wuseless-cast
        -Wsuggest-override>
    $<$<CONFIG:Debug>:-Og -g3 -ggdb>
    $<$<CONFIG:Release>:-Og -g0>
)

target_link_options(${EXECUTABLE} PRIVATE
    -T${MCU_LINKER_SCRIPT}
    ${CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    --specs=nosys.specs
    -Wl,--start-group
    -lc
    -lm
    -lstdc++
    -Wl,--end-group
    -Wl,--print-memory-usage
)

target_link_libraries(
    ${EXECUTABLE} 
    FreeRTOS
)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXECUTABLE}>
)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}>
    ${EXECUTABLE}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}>
    ${EXECUTABLE}.bin
)

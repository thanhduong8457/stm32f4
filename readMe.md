cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=true

submudule:
    url = https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    url = https://github.com/thanhduong8457/STM32F4_Driver.git

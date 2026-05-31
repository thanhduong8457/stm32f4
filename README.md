# adas_mcu_dev

STM32 FreeRTOS firmware refactored into a C++ object-oriented architecture with target-selectable platform configuration.

## Layout

- `include/app`, `src/app`: application managers and tasks
- `include/hal`: hardware-facing C++ interfaces
- `include/middleware`: FreeRTOS helper wrappers
- `platform/stm32f1`: STM32F103 startup, linker script, board composition, StdPeriph drivers
- `platform/stm32f4`: STM32F407 startup, linker script, board composition, StdPeriph drivers
- `FreeRTOS-Kernel`, `stm32f10x-stdperiph-lib`, `STM32F4_Driver`: external RTOS/vendor code

## STM32F1 Hardware Mapping

| Pin | Function |
| --- | --- |
| PC13 | Status LED |
| PB9 | TIM4 CH4 servo/PWM output |
| PA9 | USART1 TX, 115200 8N1 |
| PA10 | USART1 RX, 115200 8N1 |
| PA6 | TIM3 CH1 encoder A |
| PA7 | TIM3 CH2 encoder B |

## STM32F4 Hardware Mapping

| Pin | Function |
| --- | --- |
| PC13 | Status LED |
| PB9 | TIM4 CH4 servo/PWM output, GPIO AF2 |
| PA9 | USART1 TX, 115200 8N1, GPIO AF7 |
| PA10 | USART1 RX, 115200 8N1, GPIO AF7 |
| PA6 | TIM3 CH1 encoder A, GPIO AF2 |
| PA7 | TIM3 CH2 encoder B, GPIO AF2 |

The STM32F407 linker script uses 1 MB FLASH at `0x08000000`, 128 KB RAM at
`0x20000000`, and 64 KB CCMRAM at `0x10000000`.

## UART Service Mode

USART1 accepts newline-terminated ASCII at 115200 8N1. Configuration commands
are only accepted in service mode.

Enter service mode:

```text
SERVICE MODE
```

Exit service mode:

```text
exit
```

Supported service-mode commands:

```text
SET <0..180>  Set the motor target angle in degrees.
STOP          Disable motor output and clear the target.
STATUS        Print current target, enabled state, encoder position, and delta.
```

Commands are validated before changes are applied. Invalid syntax, out-of-range
targets, or a busy application queue return an `ERR ...` response.

## LED Behavior

- Normal mode: PC13 toggles every 1 second.
- Entering service mode: normal blinking stops and PC13 is turned off.
- Service mode: successful setting commands blink PC13 2 times, then leave it off.
- Service mode: invalid or failed commands blink PC13 3 times, then leave it off.

## Example UART Flow

```text
> SERVICE MODE
< OK service mode entered
> SET 90
< OK target accepted
> SET 220
< ERR target range 0..180
> STATUS
< STATUS target=90 enabled=1 encoder=0 delta=0
> STOP
< OK motor stopped
> exit
< OK service mode exited
```

## Build

```bash
make STM32_TARGET=stm32f1
make STM32_TARGET=stm32f4
make BUILD_TYPE=Release STM32_TARGET=stm32f1
```

Equivalent direct CMake flow with separate build directories:

```bash
cmake -S . -Bbuild-f1 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake \
  -DSTM32_TARGET=stm32f1 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-f1

cmake -S . -Bbuild-f4 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake \
  -DSTM32_TARGET=stm32f4 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-f4
```

## Flash

```bash
make flash
```

This writes `build/adas_mcu_dev.bin` to `0x08000000` using `st-flash`.
When using a separate build directory, flash the matching `.bin` manually or
adjust `BUILD_DIR`.

# adas_mcu_dev

STM32 FreeRTOS firmware refactored into a C++ object-oriented architecture with target-selectable platform configuration.

## Layout

- `src/app`: application managers, PID controller, runtime configuration, and tasks
- `include/hal`: hardware-facing C++ interfaces
- `include/middleware`: FreeRTOS helper wrappers
- `platform/stm32f1`: STM32F103 startup, linker script, board composition, StdPeriph drivers
- `platform/stm32f4`: STM32F407 startup, linker script, board composition, StdPeriph drivers
- `FreeRTOS-Kernel`, `stm32f10x-stdperiph-lib`, `STM32F4_Driver`: external RTOS/vendor code

## STM32F1 Hardware Mapping

| Pin | Function |
| --- | --- |
| PC13 | Status LED |
| PB9 | TIM4 CH4 motor PWM duty output |
| PA9 | USART1 TX, 115200 8N1 |
| PA10 | USART1 RX, 115200 8N1 |
| PA6 | TIM3 CH1 encoder A |
| PA7 | TIM3 CH2 encoder B |

## STM32F4 Hardware Mapping

| Pin | Function |
| --- | --- |
| PC13 | Status LED |
| PB9 | TIM4 CH4 motor PWM duty output, GPIO AF2 |
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
SET RPM <0..10000>         Set target motor speed.
SET KP <0.000..100.000>   Set proportional gain.
SET KI <0.000..100.000>   Set integral gain.
SET KD <0.000..100.000>   Set derivative gain.
SET SAMPLE_TIME <5..1000> Set encoder/PID loop period in ms.
GET RPM                   Read target RPM.
GET KP                    Read proportional gain.
GET KI                    Read integral gain.
GET KD                    Read derivative gain.
GET SAMPLE_TIME           Read sample/control period.
SAVE CONFIG               Save active parameters to the runtime config shadow.
LOAD CONFIG               Load parameters from the runtime config shadow.
RESET PID                 Clear PID integral, derivative, and soft-start state.
STATUS                    Print RPM, duty, direction, PID, encoder, and state.
STOP                      Disable motor output and clear target RPM.
```

Commands are validated before changes are applied. Invalid syntax, out-of-range
targets, or a busy application queue return an `ERR ...` response.

## Encoder And Control Loop

TIM3 runs in hardware quadrature encoder mode on both STM32F1 and STM32F4
targets using PA6 as Encoder A and PA7 as Encoder B. `EncoderManager` samples
the 16-bit timer count with wrap-safe deltas and maintains a signed cumulative
pulse count.

RPM is calculated every `SAMPLE_TIME` milliseconds:

```text
rpm = abs(delta_counts) * 60000 / (encoder_counts_per_revolution * sample_time_ms)
```

Direction is `CW` for positive deltas, `CCW` for negative deltas, and `STOPPED`
when no pulses arrive during the sample. The default encoder scale is
`1024` counts per revolution in `src/app/app_config.hpp`.

`MotorController` owns the deterministic speed loop. It drains command messages
without blocking, runs the PID update with `vTaskDelayUntil`, soft-starts target
RPM when enabled, saturates output to `0..1000` duty permille, and writes only
PWM duty updates to TIM4 CH4.

`SAVE CONFIG` and `LOAD CONFIG` use `ConfigurationManager`'s RAM-backed shadow
store. The storage API is isolated so a target-specific Flash/EEPROM backend can
replace the volatile shadow later.

## LED Behavior

- Normal mode: PC13 toggles every 1 second.
- Entering service mode: normal blinking stops and PC13 is turned off.
- Service mode: successful setting commands blink PC13 2 times, then leave it off.
- Service mode: invalid or failed commands blink PC13 3 times, then leave it off.

## Example UART Flow

```text
> SERVICE MODE
< OK service mode entered
> SET RPM 1500
< OK
> SET KP 1.2
< OK
> SET KI 0.08
< OK
> SET KD 0.02
< OK
> GET KP
< KP 1.200
> SET SAMPLE_TIME 20
< OK
> STATUS
< STATUS current_rpm=0 target_rpm=1500 pwm_duty=0 direction=STOPPED kp=1.200 ki=0.080 kd=0.020 pid_output=0 encoder_count=0 controller=ENABLED
> SAVE CONFIG
< OK
> STOP
< OK
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

# STM32 RTOS C++ Architecture

## Layers

The firmware is now split into explicit ownership layers:

| Layer | Folder | Responsibility |
| --- | --- | --- |
| Application | `include/app`, `src/app` | RTOS tasks, command parsing, system state, motor/encoder workflow |
| Middleware | `include/middleware` | Small FreeRTOS wrappers used by application classes |
| HAL interfaces | `include/hal` | C++ contracts for UART, servo/PWM, encoder, GPIO output, hardware components |
| Platform | `platform/stm32f1`, `platform/stm32f4` | Chip-specific startup, linker, StdPeriph/CMSIS setup, concrete peripheral drivers |
| RTOS/vendor | `FreeRTOS-Kernel`, `stm32f10x-stdperiph-lib`, `STM32F4_Driver` | Third-party kernel and STM32 vendor code |

Application classes use constructor injection and depend on `hal::` interfaces, not STM32 registers. Each board composes concrete drivers and application objects in its own `platform/<target>/board.cpp`.

## Architecture Diagrams

### Layered View

```mermaid
flowchart TB
    User[UART user or host tool]
    Main[main.cpp]
    SelectedBoard[platform::selectedBoard]
    Application[app::Application]

    subgraph AppLayer[Application layer]
        InterfaceManager[app::InterfaceManager]
        DirectorManager[app::DirectorManager]
        MotorController[app::MotorController]
        EncoderManager[app::EncoderManager]
        BlinkTask[app::BlinkTask]
        Messages[app::SystemMessage and command structs]
    end

    subgraph MiddlewareLayer[Middleware layer]
        RtosQueue[middleware::RtosQueue]
        FreeRTOSApi[FreeRTOS tasks and queues]
    end

    subgraph HalLayer[HAL interface layer]
        IUart[hal::IUart]
        IServoOutput[hal::IServoOutput]
        IEncoder[hal::IEncoder]
        IDigitalOutput[hal::IDigitalOutput]
    end

    subgraph PlatformLayer[Selected platform implementation]
        F1Board[platform::stm32f1::Board and drivers]
        F4Board[platform::stm32f4::Board and drivers]
    end

    subgraph VendorLayer[RTOS and vendor code]
        FreeRTOS[FreeRTOS-Kernel]
        F1Vendor[STM32F10x StdPeriph and CMSIS]
        F4Vendor[STM32F4 StdPeriph and CMSIS]
    end

    User --> InterfaceManager
    Main --> SelectedBoard
    SelectedBoard --> Application
    Application --> InterfaceManager
    Application --> DirectorManager
    Application --> MotorController
    Application --> EncoderManager
    Application --> BlinkTask

    InterfaceManager --> Messages
    DirectorManager --> Messages
    MotorController --> Messages
    EncoderManager --> Messages

    InterfaceManager --> RtosQueue
    MotorController --> RtosQueue
    RtosQueue --> FreeRTOSApi
    FreeRTOSApi --> FreeRTOS

    InterfaceManager --> IUart
    MotorController --> IServoOutput
    EncoderManager --> IEncoder
    BlinkTask --> IDigitalOutput

    F1Board -. implements .-> IUart
    F1Board -. implements .-> IServoOutput
    F1Board -. implements .-> IEncoder
    F1Board -. implements .-> IDigitalOutput

    F4Board -. implements .-> IUart
    F4Board -. implements .-> IServoOutput
    F4Board -. implements .-> IEncoder
    F4Board -. implements .-> IDigitalOutput

    F1Board --> F1Vendor
    F4Board --> F4Vendor
```

### Runtime Message Flow

```mermaid
sequenceDiagram
    participant Host as UART host
    participant ISR as USART1_IRQHandler
    participant Interface as InterfaceManager
    participant Director as DirectorManager
    participant Motor as MotorController
    participant Servo as IServoOutput
    participant Encoder as EncoderManager
    participant Led as BlinkTask

    Host->>ISR: ASCII bytes on USART1
    ISR->>Interface: onRxByteFromIsr(byte)
    Interface->>Interface: Build command line
    Interface->>Director: Parsed SystemCommand

    alt SET command
        Director->>Motor: MotorCommand SetMotorTarget
        Motor->>Motor: Clamp target and update state
        Motor->>Servo: setAngleDegrees(target)
    else STOP command
        Director->>Motor: MotorCommand StopMotor
        Motor->>Servo: setAngleDegrees(0)
    else STATUS command
        Director->>Interface: Send status response
        Interface->>Host: UART text response
    end

    loop periodic tasks
        Encoder->>Director: EncoderFeedback
        Led->>Led: Toggle IDigitalOutput
    end
```

### Build Target Selection

```mermaid
flowchart LR
    CMake[Configure with STM32_TARGET]
    F1[stm32f1]
    F4[stm32f4]
    F1Target[platform/stm32f1/target.cmake]
    F4Target[platform/stm32f4/target.cmake]
    F1Image[adas_mcu_dev for STM32F103]
    F4Image[adas_mcu_dev for STM32F407]

    CMake --> F1
    CMake --> F4
    F1 --> F1Target
    F4 --> F4Target
    F1Target --> F1Image
    F4Target --> F4Image
```

## Key Classes

| Class | Role |
| --- | --- |
| `app::Application` | Initializes managers and creates FreeRTOS tasks |
| `app::DirectorManager` | Owns system state, validates commands, routes motor commands |
| `app::InterfaceManager` | Owns UART RX queue, line parser, and responses |
| `app::MotorController` | Owns motor queue and servo output policy |
| `app::EncoderManager` | Samples encoder feedback and reports to Director |
| `app::BlinkTask` | Toggles the status LED through `hal::IDigitalOutput` |
| `platform::stm32f1::Board` | Wires STM32F1 drivers into the application graph |
| `platform::stm32f4::Board` | Wires STM32F407 drivers into the application graph |

## Adding Hardware

1. Add or reuse an interface in `include/hal`.
2. Implement the concrete driver in `platform/<target>`.
3. Compose the driver into that target's `Board`.
4. Pass the interface into the app class that needs it.

This keeps new peripherals out of `main.cpp` and avoids spreading chip-specific headers through application code.

## Multi-Target Build

Select the MCU family at configure time:

```bash
cmake -Bbuild \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake \
  -DSTM32_TARGET=stm32f1
cmake --build build
```

or with the Makefile:

```bash
make STM32_TARGET=stm32f1
make STM32_TARGET=stm32f4
```

`platform/stm32f1/target.cmake` provides the F103 configuration. `platform/stm32f4/target.cmake` provides the F407 configuration using the STM32F4 StdPeriph/CMSIS package and the `GCC_ARM_CM4F` FreeRTOS port.

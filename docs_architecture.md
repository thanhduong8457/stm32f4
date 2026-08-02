# STM32 RTOS C++ Architecture

## Layers

The firmware is now split into explicit ownership layers:

| Layer | Folder | Responsibility |
| --- | --- | --- |
| Application | `src/app` | RTOS managers, service-mode command parsing, runtime configuration, encoder feedback, PID speed control |
| Middleware | `include/middleware` | Small FreeRTOS wrappers used by application classes |
| HAL interfaces | `include/hal` | C++ contracts for UART, PWM duty output, encoder, GPIO output, hardware components |
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
        CEO[app::CEO]
        ConfigManager[app::ConfigurationManager]
        PidManager[app::PidManager]
        PidController[app::PidController]
        SpeedState[app::SpeedControlStateMachine]
        MotorController[app::MotorController]
        EncoderManager[app::EncoderManager]
        UIManager[app::UIManager]
        Messages[app::SystemMessage and command structs]
    end

    subgraph MiddlewareLayer[Middleware layer]
        RtosQueue[middleware::RtosQueue]
        FreeRTOSApi[FreeRTOS tasks and queues]
    end

    subgraph HalLayer[HAL interface layer]
        IUart[hal::IUart]
        IPwmOutput[hal::IPwmOutput]
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
    Application --> CEO
    Application --> MotorController
    Application --> PidManager
    Application --> EncoderManager
    Application --> UIManager

    InterfaceManager --> Messages
    InterfaceManager --> ConfigManager
    CEO --> Messages
    PidManager --> Messages
    PidManager --> PidController
    PidManager --> SpeedState
    MotorController --> Messages
    EncoderManager --> Messages

    InterfaceManager --> RtosQueue
    CEO --> RtosQueue
    PidManager --> RtosQueue
    MotorController --> RtosQueue
    EncoderManager --> RtosQueue
    UIManager --> RtosQueue
    RtosQueue --> FreeRTOSApi
    FreeRTOSApi --> FreeRTOS

    InterfaceManager --> IUart
    MotorController --> IPwmOutput
    EncoderManager --> IEncoder
    UIManager --> IDigitalOutput

    F1Board -. implements .-> IUart
    F1Board -. implements .-> IPwmOutput
    F1Board -. implements .-> IEncoder
    F1Board -. implements .-> IDigitalOutput

    F4Board -. implements .-> IUart
    F4Board -. implements .-> IPwmOutput
    F4Board -. implements .-> IEncoder
    F4Board -. implements .-> IDigitalOutput

    F1Board --> F1Vendor
    F4Board --> F4Vendor
```

### Service Mode And PID Tuning Flow

```mermaid
sequenceDiagram
    participant Host as UART host
    participant ISR as USART1_IRQHandler
    participant Interface as InterfaceManager
    participant Director as CEO
    participant Config as ConfigurationManager
    participant PID as PidManager
    participant Led as UIManager

    Host->>ISR: SERVICE MODE bytes on USART1
    ISR->>Interface: onRxByteFromIsr(byte)
    Interface->>Interface: Build command line
    Interface->>Director: ServiceModeEntered event
    Director->>Led: ServiceMode event
    Led->>Led: Stop normal blink and set LED off
    Interface->>Host: OK service mode entered

    Host->>ISR: SET KP 1.200 bytes
    ISR->>Interface: onRxByteFromIsr(byte)
    Interface->>Interface: Parse fixed-point value
    Interface->>Config: Validate requested KP
    Interface->>Director: SystemCommand::SetKp
    alt PID queue accepted
        Director->>PID: PidCommand SetKp
        Director->>Interface: CommandOk event
        Interface->>Config: Update active KP
        Interface->>Host: OK
        Director->>Led: SettingSucceeded event
    else invalid value
        Interface->>Host: ERR invalid KP
        Interface->>Director: SettingFailed event
        Director->>Led: SettingFailed event
    else manager queue busy
        Director->>Interface: ManagerBusy event
        Interface->>Host: ERR manager busy
        Director->>Led: SettingFailed event
    end

    Host->>ISR: exit bytes
    ISR->>Interface: onRxByteFromIsr(byte)
    Interface->>Director: ServiceModeExited event
    Director->>Led: NormalMode event
    Interface->>Host: OK service mode exited

    Led->>Led: Resume 1-second normal blink
```

### Closed-Loop Control Flow

```mermaid
sequenceDiagram
    participant EncoderHw as TIM3 encoder mode
    participant Encoder as EncoderManager
    participant Director as CEO
    participant PIDMgr as PidManager
    participant PID as PidController
    participant Motor as MotorController
    participant PWM as TIM4 CH4 PWM

    loop every SAMPLE_TIME ms
        Encoder->>EncoderHw: Read quadrature count
        Encoder->>Encoder: delta = int16(raw - previous)
        Encoder->>Encoder: rpm = abs(delta) * 60000 / (CPR * sample_ms)
        Encoder->>Director: EncoderFeedback(count, delta, rpm, direction)
        Director->>PIDMgr: PidCommand SetActualRpm
    end

    loop every PID control period
        PIDMgr->>PIDMgr: Drain queued PID commands without blocking
        PIDMgr->>PIDMgr: Check feedback watchdog and soft-start target RPM
        PIDMgr->>PID: update(ramped target RPM, actual RPM)
        PID->>PID: Feed-forward + PID correction, filtered D, anti-windup
        PID-->>PIDMgr: duty permille 0..1000
        PIDMgr->>Director: PidOutput(duty, enabled)
        Director->>Motor: MotorCommand Enable/Disable and SetDuty
        Motor->>PWM: setDutyCyclePermille(duty)
    end
```

### Status Request Flow

```mermaid
sequenceDiagram
    participant Host as UART host
    participant Interface as InterfaceManager
    participant Director as CEO
    participant Encoder as EncoderManager
    participant PID as PidManager
    participant Motor as MotorController

    Host->>Interface: STATUS
    Interface->>Director: StatusRequest
    Director->>Encoder: Request status snapshot
    Director->>PID: Request status snapshot
    Director->>Motor: Request status snapshot
    Director->>Interface: Status event
    Interface->>Host: UART status response
```

### State Machine

```mermaid
stateDiagram-v2
    state ServiceWorkflow {
        [*] --> NormalMode
        NormalMode: LED toggles every 1 second
        NormalMode --> ServiceMode: UART SERVICE MODE
        ServiceMode: LED off, UART config enabled
        ServiceMode --> CommandOk: Valid command applied
        ServiceMode --> CommandError: Invalid command or failed queue
        CommandOk: Blink LED 2 times
        CommandError: Blink LED 3 times
        CommandOk --> ServiceMode
        CommandError --> ServiceMode
        ServiceMode --> NormalMode: UART exit
    }

    state MotorControl {
        [*] --> MotorStopped
        MotorStopped --> AwaitingFeedback: SET RPM > 0, no feedback seen
        MotorStopped --> SoftStart: SET RPM > 0, feedback healthy
        AwaitingFeedback: PWM forced to zero
        AwaitingFeedback --> SoftStart: Fresh encoder feedback
        SoftStart --> ClosedLoop: ramped target reaches target RPM
        ClosedLoop --> ClosedLoop: Encoder feedback + PID update
        SoftStart --> FeedbackFault: Feedback stale > 5 periods
        ClosedLoop --> FeedbackFault: Feedback stale > 5 periods
        FeedbackFault: PWM forced to zero, PID state reset
        FeedbackFault --> SoftStart: Feedback recovers
        ClosedLoop --> MotorStopped: STOP or SET RPM 0
        SoftStart --> MotorStopped: STOP or SET RPM 0
        AwaitingFeedback --> MotorStopped: STOP or SET RPM 0
        FeedbackFault --> MotorStopped: STOP or SET RPM 0
    }
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
| `app::CEO` | Orchestrates manager requests/events, routes commands, coordinates workflows, and monitors manager status snapshots |
| `app::InterfaceManager` | Owns the UART/CDC RX queue, service-mode state, parameter validation, runtime config shadow, and responses |
| `app::parseInterfaceCommand` | Pure host-command parser with bounded integer/fixed-point conversion; independently host-testable |
| `app::ConfigurationManager` | Helper owned by InterfaceManager for validated active config and RAM-backed saved config shadow |
| `app::PidManager` | Owns the periodic speed-control task, target RPM, feedback watchdog, soft-start, and PWM output events |
| `app::PidController` | Pure feed-forward-assisted PID math with filtered derivative-on-measurement, conditional anti-windup, and output limits |
| `app::SpeedControlStateMachine` | Pure formal state owner for stopped, feedback wait/fault, soft-start, and closed-loop transitions |
| `app::MotorController` | Owns motor enable/disable and PWM duty output policy; it performs no PID calculations |
| `app::EncoderManager` | Samples TIM encoder counts, calculates pulse count/direction/RPM, and reports feedback to CEO |
| `app::UIManager` | Owns normal blinking, service-mode LED-off state, and success/failure status blink patterns |
| `platform::stm32f1::Board` | Wires STM32F1 drivers into the application graph |
| `platform::stm32f4::Board` | Wires STM32F407 drivers into the application graph |

## Runtime Configuration

`InterfaceManager` owns `ConfigurationManager`, which keeps one active
configuration and one saved shadow:

| Parameter | Range | Default | UART |
| --- | --- | --- | --- |
| Target RPM | `0..10000` | `0` | `SET RPM`, `GET RPM` |
| Kp | `0.000..100.000` | `1.200` | `SET KP`, `GET KP` |
| Ki | `0.000..100.000` | `0.080` | `SET KI`, `GET KI` |
| Kd | `0.000..100.000` | `0.020` | `SET KD`, `GET KD` |
| Sample/control time | `5..1000 ms` | `20 ms` | `SET SAMPLE_TIME`, `GET SAMPLE_TIME` |

PID gains are parsed and stored as fixed-point milli-units, so `1.200` is held
as `1200`. `SAVE CONFIG` copies active values to the shadow; `LOAD CONFIG`
restores the shadow and sends a load request through CEO so CEO can route values
to PidManager and EncoderManager. The current storage backend is volatile RAM
by design; a target-specific Flash/EEPROM implementation can replace the shadow
without changing the UART or control-loop APIs.

## Encoder RPM Calculation

Both STM32 targets use TIM3 hardware encoder mode with PA6/PA7 as quadrature A/B
inputs. The portable encoder task reads the raw 16-bit timer count and computes:

```text
delta_counts = int16(current_raw_count - previous_raw_count)
encoder_count += delta_counts
rpm = abs(delta_counts) * 60000 / (encoder_counts_per_revolution * sample_time_ms)
```

Positive deltas are reported as `CW`, negative deltas as `CCW`, and zero deltas
as `STOPPED`. The default scale is `1024` counts per revolution in
`src/app/app_config.hpp`.

## Control Timing

The PID manager task is the deterministic control loop. It drains pending PID
commands with zero timeout, advances the soft-start target ramp, runs PID, emits
a duty event to CEO, then sleeps with `vTaskDelayUntil(controlPeriodMs)`. CEO
routes duty events to MotorController. The UART path never performs PWM writes
directly, MotorController performs no PID calculations, and the control loop
does not print or send UART responses.

The control output is:

```text
error = ramped_target_rpm - measured_rpm
feed_forward = ramped_target_rpm * kFeedForwardDutyAtMaxRpm / kTargetRpmMax
duty = clamp(feed_forward + Kp*error + Ki*integral(error) - Kd*d(measured_rpm)/dt,
             kPwmDutyMinPermille, kPwmDutyMaxPermille)
```

The feed-forward path is a simple motor model: it maps desired RPM directly to
an approximate steady PWM. PID is retained as the feedback correction for load,
battery, friction, and model errors. This is preferable to error-only PID here
because it reduces startup lag and the amount of integral required. Derivative
uses measured RPM and an `0.2` new / `0.8` previous fixed-point low-pass filter,
so a target step does not create a derivative kick. The integrator is symmetric
around zero and conditionally freezes when its next step would push a saturated
output farther into saturation.

`PidManager` also supervises the encoder message stream. PWM stays disabled
until at least one fresh RPM sample arrives. More than
`kEncoderFeedbackTimeoutPeriods` missed control periods marks feedback stale,
sets duty to zero, and resets PID/soft-start state. Fresh feedback automatically
restarts control through the ramp. This watchdog detects a stopped encoder task
or broken message path; a disconnected encoder that still produces valid zero
samples requires separate hardware-specific stall/current protection.

`SpeedControlStateMachine` owns the transitions shown in the motor-control state
diagram. `PidManager` supplies only three facts—whether a nonzero target exists,
whether feedback is healthy, and whether feedback has ever been seen—plus the
soft-start ramp completion. This keeps fault/recovery policy in one pure class
that can be tested on the host independently of FreeRTOS.

## Beta/LegoPlus Concept Mapping

The architecture was reviewed against the sibling reference firmware under
`../../beta`. That firmware does not provide a motor
PID to port. The reusable ideas come from its LegoPlus framework documentation
and interfaces: `IDirector`, `IManager`, `CAQueueManager`, `IDriver`,
`IDiagnostic`, `ICompAssistant`, shared objects, and the flash-based state
machine.

| LegoPlus concept | Current-project equivalent | Adaptation |
| --- | --- | --- |
| Director registration and notification routing | `CEO` with typed queues | Dependencies are constructor-injected instead of discovered through a singleton |
| Manager task plus notification queue | Manager `run()` methods plus `RtosQueue<T>` | Each queue has a specific value type and bounded capacity |
| Driver/logical-port abstraction | `hal::` interfaces and platform drivers | Interfaces are capability-sized rather than generic property bags |
| Formal flash-based state machine | `SpeedControlStateMachine` | Transitions are explicit C++ states and host-tested; no dynamic tables are required |
| Runtime manager status and `IDiagnostic` | `Status`, `SystemStatus`, feedback health, and `control_state` | Read-only snapshots report safety state without perturbing control |
| Component assistants and factory | Target `Board` plus `Application` composition | The fixed build graph is resolved at compile time |

The global component factory, custom permanent/semi-permanent/temporary heaps,
and polymorphic shared-object routing are intentionally omitted. They solve
dynamic product-family composition in beta, while this firmware benefits more
from static allocation, explicit ownership, and compile-time type checking.

## Controller Selection And Tuning

The implementation supports full PID, but feed-forward + PI is the recommended
starting point for a DC motor speed loop. Encoder differentiation amplifies
quantization and mechanical noise, so `KD` should remain zero unless bench tests
show that additional damping is useful.

1. Set `kEncoderCountsPerRevolution` to the count seen at the controlled shaft,
   including quadrature multiplication and gearbox ratio.
2. Measure no-load speed near full duty and use it for `kTargetRpmMax`; adjust
   `kFeedForwardDutyAtMaxRpm` only when full-scale allowed duty is below 1000.
3. Tune `KP` with `KI=0`, `KD=0`; raise it to a responsive but non-oscillatory
   value and then leave stability margin.
4. Raise `KI` gradually to reject load-induced steady-state error.
5. Add a small `KD` only when needed, watching RPM noise and duty jitter.
6. Validate target steps, load steps, saturation recovery, STOP, and feedback
   loss using a current-limited bench supply.

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

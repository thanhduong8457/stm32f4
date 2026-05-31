#pragma once

#include <cstdint>

namespace app
{

/**
 * Commands passed between application managers.
 *
 * The values are stable message IDs rather than bit flags. Managers switch on
 * these IDs to decide how the payload in the matching message structure should
 * be interpreted.
 */
enum class SystemCommand : uint8_t
{
    SetTargetRpm = 1,
    StopMotor,
    EncoderFeedback,
    PidOutput,
    StatusRequest,
    SetKp,
    SetKi,
    SetKd,
    SetSampleTime,
    GetTargetRpm,
    GetKp,
    GetKi,
    GetKd,
    GetSampleTime,
    SaveConfig,
    LoadConfig,
    ResetPid,
    ServiceModeEntered,
    ServiceModeExited,
    SettingSucceeded,
    SettingFailed,
    InvalidCommand,
};

/**
 * Origin of a message in the application graph.
 *
 * Keeping the source in messages makes debug output and routing decisions easier
 * without coupling managers to concrete platform drivers.
 */
enum class MessageSource : uint8_t
{
    Interface = 1,
    Director,
    Motor,
    Encoder,
    Pid,
};

enum class RotationDirection : int8_t
{
    Stopped = 0,
    Cw = 1,
    Ccw = -1,
};

enum class MotorCommandType : uint8_t
{
    SetDuty = 1,
    Enable,
    Disable,
};

enum class PidCommandType : uint8_t
{
    SetTargetRpm = 1,
    SetActualRpm,
    SetKp,
    SetKi,
    SetKd,
    SetControlPeriod,
    Reset,
};

enum class EncoderCommandType : uint8_t
{
    SetSamplePeriod = 1,
};

/**
 * Generic cross-manager message.
 *
 * This is useful when a task needs to report a command-like event plus one
 * integer payload, for example a parsed target value or status request.
 */
struct SystemMessage
{
    SystemCommand cmd;
    MessageSource source;
    int32_t value;
    int32_t aux = 0;
    int32_t extra = 0;
    int32_t detail = 0;
    int32_t detail2 = 0;
};

/**
 * Message consumed by MotorController. MotorController only applies PWM duty
 * and enable state; it does not own speed-control algorithms.
 */
struct MotorCommand
{
    MotorCommandType type;
    int32_t value;
};

/**
 * Message consumed by PidManager. The value field carries target RPM, actual
 * RPM, sample time, or PID gain milli-units depending on type.
 */
struct PidCommand
{
    PidCommandType type;
    int32_t value;
};

/**
 * Message consumed by EncoderManager.
 */
struct EncoderCommand
{
    EncoderCommandType type;
    int32_t value;
};

/**
 * Encoder sample delivered to higher-level logic.
 */
struct EncoderFeedback
{
    int32_t count;
    int32_t delta;
    int32_t rpm;
    RotationDirection direction;
};

struct SystemStatus
{
    int32_t currentRpm = 0;
    int32_t targetRpm = 0;
    int32_t pwmDutyPermille = 0;
    RotationDirection direction = RotationDirection::Stopped;
    int32_t kp = 0;
    int32_t ki = 0;
    int32_t kd = 0;
    int32_t pidOutput = 0;
    int32_t encoderCount = 0;
    bool controllerEnabled = false;
};

enum class InterfaceEventType : uint8_t
{
    CommandOk = 1,
    ManagerBusy,
    Unsupported,
    Status,
};

struct InterfaceEvent
{
    InterfaceEventType type;
    SystemCommand command = SystemCommand::InvalidCommand;
    int32_t value = 0;
    int32_t aux = 0;
    int32_t extra = 0;
    int32_t detail = 0;
    int32_t detail2 = 0;
    SystemStatus status{};
};

} // namespace app

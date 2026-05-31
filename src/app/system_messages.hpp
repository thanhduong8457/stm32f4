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
};

enum class RotationDirection : int8_t
{
    Stopped = 0,
    Cw = 1,
    Ccw = -1,
};

enum class MotorCommandType : uint8_t
{
    SetTargetRpm = 1,
    Stop,
    SetActualRpm,
    SetKp,
    SetKi,
    SetKd,
    SetControlPeriod,
    ResetPid,
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
};

/**
 * Message consumed by MotorController. The value field carries the primary
 * payload for the selected type, for example target RPM, actual RPM, or a PID
 * gain in milli-units.
 */
struct MotorCommand
{
    MotorCommandType type;
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

} // namespace app

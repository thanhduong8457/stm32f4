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
    SetMotorTarget = 1,
    StopMotor,
    EncoderFeedback,
    StatusRequest,
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
};

/**
 * Message consumed by MotorController.
 *
 * SetMotorTarget uses target as a requested servo angle. StopMotor ignores the
 * target field and disables output until a new target command arrives.
 */
struct MotorCommand
{
    SystemCommand cmd;
    int32_t target;
};

/**
 * Encoder sample delivered to higher-level logic.
 *
 * position is the current counter-derived position and delta is the movement
 * since the previous sample.
 */
struct EncoderFeedback
{
    int32_t position;
    int32_t delta;
};

} // namespace app

#pragma once

#include <cstdint>

namespace app
{

enum class SystemCommand : uint8_t
{
    SetMotorTarget = 1,
    StopMotor,
    EncoderFeedback,
    StatusRequest,
    InvalidCommand,
};

enum class MessageSource : uint8_t
{
    Interface = 1,
    Director,
    Motor,
    Encoder,
};

struct SystemMessage
{
    SystemCommand cmd;
    MessageSource source;
    int32_t value;
};

struct MotorCommand
{
    SystemCommand cmd;
    int32_t target;
};

struct EncoderFeedback
{
    int32_t position;
    int32_t delta;
};

} // namespace app

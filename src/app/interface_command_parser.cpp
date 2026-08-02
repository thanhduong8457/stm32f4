#include "app/interface_command_parser.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

#include "app/app_config.hpp"

namespace app
{
namespace
{

bool isSpace(char value)
{
    return value == ' ' || value == '\t' || value == '\r' || value == '\n' || value == '\f' ||
           value == '\v';
}

bool isDigit(char value)
{
    return value >= '0' && value <= '9';
}

void skipSpaces(const char *&text)
{
    while (isSpace(*text))
    {
        ++text;
    }
}

bool trailingSpacesOnly(const char *text)
{
    while (*text != '\0')
    {
        if (!isSpace(*text))
        {
            return false;
        }
        ++text;
    }
    return true;
}

bool textEquals(const char *left, const char *right, std::size_t length)
{
    for (std::size_t index = 0; index < length; ++index)
    {
        if (left[index] != right[index])
        {
            return false;
        }
    }
    return true;
}

std::size_t textLength(const char *text)
{
    std::size_t length = 0;
    while (text[length] != '\0')
    {
        ++length;
    }
    return length;
}

bool isCommand(const char *text, const char *command)
{
    const std::size_t commandLength = textLength(command);
    return textEquals(text, command, commandLength) && trailingSpacesOnly(text + commandLength);
}

bool consumeCommand(const char *&text, const char *command)
{
    const std::size_t commandLength = textLength(command);
    if (!textEquals(text, command, commandLength) || !isSpace(text[commandLength]))
    {
        return false;
    }

    text += commandLength;
    skipSpaces(text);
    return true;
}

bool parseInteger(const char *text, int32_t &value)
{
    skipSpaces(text);

    bool negative = false;
    if (*text == '+' || *text == '-')
    {
        negative = *text == '-';
        ++text;
    }

    if (!isDigit(*text))
    {
        return false;
    }

    const uint32_t limit = negative
                               ? (static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) + 1U)
                               : static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
    uint32_t parsed = 0;
    while (isDigit(*text))
    {
        const uint32_t digit = static_cast<uint32_t>(*text - '0');
        if (parsed > ((limit - digit) / 10U))
        {
            return false;
        }
        parsed = (parsed * 10U) + digit;
        ++text;
    }

    if (!trailingSpacesOnly(text))
    {
        return false;
    }

    if (negative && parsed == (static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) + 1U))
    {
        value = std::numeric_limits<int32_t>::min();
    }
    else
    {
        value = negative ? -static_cast<int32_t>(parsed) : static_cast<int32_t>(parsed);
    }
    return true;
}

bool parseFixedMilli(const char *text, int32_t &value)
{
    skipSpaces(text);

    uint32_t whole = 0;
    bool hasDigit = false;
    while (isDigit(*text))
    {
        hasDigit = true;
        const uint32_t digit = static_cast<uint32_t>(*text - '0');
        const uint32_t wholeLimit =
            static_cast<uint32_t>(std::numeric_limits<int32_t>::max() / config::kPidGainScale);
        if (whole > ((wholeLimit - digit) / 10U))
        {
            return false;
        }
        whole = (whole * 10U) + digit;
        ++text;
    }

    uint32_t fraction = 0;
    uint32_t scale = static_cast<uint32_t>(config::kPidGainScale / 10);
    if (*text == '.')
    {
        ++text;
        while (scale > 0U && isDigit(*text))
        {
            hasDigit = true;
            fraction += static_cast<uint32_t>(*text - '0') * scale;
            scale /= 10U;
            ++text;
        }
        if (isDigit(*text))
        {
            return false;
        }
    }

    const uint64_t parsed =
        (static_cast<uint64_t>(whole) * static_cast<uint32_t>(config::kPidGainScale)) + fraction;
    if (!hasDigit || !trailingSpacesOnly(text) ||
        parsed > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
    {
        return false;
    }

    value = static_cast<int32_t>(parsed);
    return true;
}

ParsedInterfaceCommand error(const char *response, bool reportFailure)
{
    ParsedInterfaceCommand parsed{};
    parsed.response = response;
    parsed.reportFailure = reportFailure;
    return parsed;
}

ParsedInterfaceCommand directAction(InterfaceCommandAction action)
{
    ParsedInterfaceCommand parsed{};
    parsed.action = action;
    return parsed;
}

ParsedInterfaceCommand directorCommand(SystemCommand command, int32_t value = 0)
{
    ParsedInterfaceCommand parsed{};
    parsed.action = InterfaceCommandAction::SendToDirector;
    parsed.message = SystemMessage{command, MessageSource::Interface, value};
    return parsed;
}

ParsedInterfaceCommand parameterCommand(SystemCommand command, ConfigParameter parameter,
                                        int32_t value)
{
    ParsedInterfaceCommand parsed = directorCommand(command, value);
    parsed.parameter = parameter;
    parsed.validateParameter = true;
    return parsed;
}

ParsedInterfaceCommand parseSetCommand(const char *text)
{
    int32_t value = 0;
    if (consumeCommand(text, "RPM"))
    {
        if (!parseInteger(text, value))
        {
            return error("ERR bad RPM value\r\n", true);
        }
        return parameterCommand(SystemCommand::SetTargetRpm, ConfigParameter::TargetRpm, value);
    }
    if (consumeCommand(text, "KP"))
    {
        if (!parseFixedMilli(text, value))
        {
            return error("ERR bad KP value\r\n", true);
        }
        return parameterCommand(SystemCommand::SetKp, ConfigParameter::Kp, value);
    }
    if (consumeCommand(text, "KI"))
    {
        if (!parseFixedMilli(text, value))
        {
            return error("ERR bad KI value\r\n", true);
        }
        return parameterCommand(SystemCommand::SetKi, ConfigParameter::Ki, value);
    }
    if (consumeCommand(text, "KD"))
    {
        if (!parseFixedMilli(text, value))
        {
            return error("ERR bad KD value\r\n", true);
        }
        return parameterCommand(SystemCommand::SetKd, ConfigParameter::Kd, value);
    }
    if (consumeCommand(text, "SAMPLE_TIME"))
    {
        if (!parseInteger(text, value))
        {
            return error("ERR bad SAMPLE_TIME value\r\n", true);
        }
        return parameterCommand(SystemCommand::SetSampleTime, ConfigParameter::SampleTime, value);
    }
    return error("ERR unsupported SET command\r\n", true);
}

ParsedInterfaceCommand parseGetCommand(const char *text)
{
    ParsedInterfaceCommand parsed = directAction(InterfaceCommandAction::GetParameter);

    if (isCommand(text, "RPM"))
    {
        parsed.parameter = ConfigParameter::TargetRpm;
    }
    else if (isCommand(text, "KP"))
    {
        parsed.parameter = ConfigParameter::Kp;
    }
    else if (isCommand(text, "KI"))
    {
        parsed.parameter = ConfigParameter::Ki;
    }
    else if (isCommand(text, "KD"))
    {
        parsed.parameter = ConfigParameter::Kd;
    }
    else if (isCommand(text, "SAMPLE_TIME"))
    {
        parsed.parameter = ConfigParameter::SampleTime;
    }
    else
    {
        return error("ERR unsupported GET command\r\n", true);
    }
    return parsed;
}

} // namespace

ParsedInterfaceCommand parseInterfaceCommand(const char *line, bool serviceMode)
{
    const char *text = line;
    skipSpaces(text);

    if (!serviceMode)
    {
        return isCommand(text, "SERVICE MODE")
                   ? directAction(InterfaceCommandAction::EnterServiceMode)
                   : error("ERR service mode required\r\n", false);
    }

    if (isCommand(text, "exit"))
    {
        return directAction(InterfaceCommandAction::ExitServiceMode);
    }
    if (isCommand(text, "SERVICE MODE"))
    {
        return directAction(InterfaceCommandAction::ServiceModeAlreadyActive);
    }
    if (consumeCommand(text, "SET"))
    {
        return parseSetCommand(text);
    }
    if (consumeCommand(text, "GET"))
    {
        return parseGetCommand(text);
    }
    if (isCommand(text, "STOP"))
    {
        return directorCommand(SystemCommand::StopMotor);
    }
    if (isCommand(text, "STATUS"))
    {
        return directorCommand(SystemCommand::StatusRequest);
    }
    if (isCommand(text, "SAVE CONFIG"))
    {
        return directAction(InterfaceCommandAction::SaveConfig);
    }
    if (isCommand(text, "LOAD CONFIG"))
    {
        return directAction(InterfaceCommandAction::LoadConfig);
    }
    if (isCommand(text, "RESET PID"))
    {
        return directorCommand(SystemCommand::ResetPid);
    }
    return error("ERR unsupported command\r\n", true);
}

} // namespace app

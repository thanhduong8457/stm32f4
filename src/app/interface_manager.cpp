#include "app/interface_manager.hpp"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/ceo.hpp"

namespace app
{
namespace
{

void trimLeadingSpaces(const char *&text)
{
    while (*text != '\0' && std::isspace(static_cast<unsigned char>(*text)) != 0)
    {
        ++text;
    }
}

bool hasOnlyTrailingSpaces(const char *text);

bool parseInteger(const char *text, int32_t &value)
{
    char *end = nullptr;
    errno = 0;
    const long parsed = std::strtol(text, &end, 10);
    if (end == text || errno == ERANGE ||
        parsed < static_cast<long>(std::numeric_limits<int32_t>::min()) ||
        parsed > static_cast<long>(std::numeric_limits<int32_t>::max()))
    {
        return false;
    }

    while (*end != '\0')
    {
        if (std::isspace(static_cast<unsigned char>(*end)) == 0)
        {
            return false;
        }
        ++end;
    }

    value = static_cast<int32_t>(parsed);
    return true;
}

bool parseFixedMilli(const char *text, int32_t &value)
{
    trimLeadingSpaces(text);

    int64_t whole = 0;
    bool hasDigit = false;
    while (std::isdigit(static_cast<unsigned char>(*text)) != 0)
    {
        hasDigit = true;
        whole = (whole * 10) + (*text - '0');
        ++text;
    }

    int32_t fraction = 0;
    int32_t scale = config::kPidGainScale / 10;
    if (*text == '.')
    {
        ++text;
        while (scale > 0 && std::isdigit(static_cast<unsigned char>(*text)) != 0)
        {
            hasDigit = true;
            fraction += (*text - '0') * scale;
            scale /= 10;
            ++text;
        }

        if (std::isdigit(static_cast<unsigned char>(*text)) != 0)
        {
            return false;
        }
    }

    if (!hasDigit || !hasOnlyTrailingSpaces(text))
    {
        return false;
    }

    const int64_t parsed = (whole * config::kPidGainScale) + fraction;
    if (parsed > std::numeric_limits<int32_t>::max())
    {
        return false;
    }

    value = static_cast<int32_t>(parsed);
    return true;
}

bool hasOnlyTrailingSpaces(const char *text)
{
    while (*text != '\0')
    {
        if (std::isspace(static_cast<unsigned char>(*text)) == 0)
        {
            return false;
        }
        ++text;
    }

    return true;
}

bool isCommandLine(const char *text, const char *command)
{
    const size_t commandLength = std::strlen(command);
    return std::strncmp(text, command, commandLength) == 0 &&
           hasOnlyTrailingSpaces(text + commandLength);
}

bool startsWithCommand(const char *text, const char *command)
{
    const size_t commandLength = std::strlen(command);
    return std::strncmp(text, command, commandLength) == 0 &&
           std::isspace(static_cast<unsigned char>(text[commandLength])) != 0;
}

const char *parameterName(ConfigParameter parameter)
{
    switch (parameter)
    {
    case ConfigParameter::TargetRpm:
        return "RPM";

    case ConfigParameter::Kp:
        return "KP";

    case ConfigParameter::Ki:
        return "KI";

    case ConfigParameter::Kd:
        return "KD";

    case ConfigParameter::SampleTime:
        return "SAMPLE_TIME";
    }

    return "UNKNOWN";
}

const char *directionText(RotationDirection direction)
{
    switch (direction)
    {
    case RotationDirection::Cw:
        return "CW";

    case RotationDirection::Ccw:
        return "CCW";

    case RotationDirection::Stopped:
    default:
        return "STOPPED";
    }
}

void formatGain(char *buffer, size_t size, int32_t gain)
{
    const int32_t whole = gain / config::kPidGainScale;
    const int32_t fraction = gain % config::kPidGainScale;
    std::snprintf(buffer, size, "%ld.%03ld", static_cast<long>(whole), static_cast<long>(fraction));
}

} // namespace

InterfaceManager::InterfaceManager(hal::IUart &uart) : uart_(uart)
{
}

void InterfaceManager::initialize(CEO &director)
{
    director_ = &director;
    rxQueue_.create(config::kInterfaceRxQueueLength);
    eventQueue_.create(config::kInterfaceEventQueueLength);
    resetPacket();
    uart_.initialize();
}

bool InterfaceManager::onRxByteFromIsr(uint8_t byte, void *higherPriorityTaskWoken)
{
    return rxQueue_.sendFromIsr(byte, static_cast<BaseType_t *>(higherPriorityTaskWoken));
}

bool InterfaceManager::sendEvent(const InterfaceEvent &event, uint32_t timeoutMs)
{
    return eventQueue_.send(event, timeoutMs);
}

void InterfaceManager::sendResponse(const char *message)
{
    uart_.send(message);
}

void InterfaceManager::processRx(char byte)
{
    if (byte == '\b' || byte == 0x7F)
    {
        if (packetLength_ > 0U)
        {
            --packetLength_;
            packet_[packetLength_] = '\0';
            sendResponse("\b \b");
        }
        return;
    }

    uart_.send(byte);

    if (byte == '\r' || byte == '\n')
    {
        if (packetLength_ > 0U)
        {
            packet_[packetLength_] = '\0';
            dispatchPacket(packet_);
            resetPacket();
        }
        return;
    }

    if (packetLength_ >= (config::kInterfacePacketMaxLength - 1U))
    {
        resetPacket();
        sendResponse("ERR packet too long\r\n");
        if (serviceMode_)
        {
            reportSettingFailure();
        }
        return;
    }

    packet_[packetLength_++] = byte;
}

void InterfaceManager::run()
{
    uint8_t byte = 0;
    InterfaceEvent event{};

    for (;;)
    {
        while (eventQueue_.receive(event, 0))
        {
            handleInterfaceEvent(event);
        }

        if (rxQueue_.receive(byte, pdMS_TO_TICKS(10)))
        {
            processRx(static_cast<char>(byte));
        }
    }
}

void InterfaceManager::resetPacket()
{
    packetLength_ = 0;
    packet_[0] = '\0';
}

void InterfaceManager::dispatchPacket(const char *packet)
{
    const char *cursor = packet;
    trimLeadingSpaces(cursor);

    if (!serviceMode_)
    {
        if (isCommandLine(cursor, "SERVICE MODE"))
        {
            serviceMode_ = true;
            (void)sendToDirector(
                SystemMessage{SystemCommand::ServiceModeEntered, MessageSource::Interface, 0});
            sendResponse("OK service mode entered\r\n");
        }
        else
        {
            sendResponse("ERR service mode required\r\n");
        }
        return;
    }

    if (isCommandLine(cursor, "exit"))
    {
        serviceMode_ = false;
        (void)sendToDirector(
            SystemMessage{SystemCommand::ServiceModeExited, MessageSource::Interface, 0});
        sendResponse("OK service mode exited\r\n");
        return;
    }

    if (isCommandLine(cursor, "SERVICE MODE"))
    {
        sendResponse("OK service mode active\r\n");
        return;
    }

    dispatchServiceCommand(cursor);
}

void InterfaceManager::dispatchServiceCommand(const char *packet)
{
    const char *cursor = packet;
    SystemMessage message{SystemCommand::InvalidCommand, MessageSource::Interface, 0};
    ConfigParameter parameter = ConfigParameter::TargetRpm;
    bool hasParameter = false;

    if (startsWithCommand(cursor, "SET"))
    {
        cursor += 3;
        trimLeadingSpaces(cursor);

        if (startsWithCommand(cursor, "RPM"))
        {
            cursor += 3;
            trimLeadingSpaces(cursor);
            if (!parseInteger(cursor, message.value))
            {
                sendResponse("ERR bad RPM value\r\n");
                reportSettingFailure();
                return;
            }
            message.cmd = SystemCommand::SetTargetRpm;
            parameter = ConfigParameter::TargetRpm;
            hasParameter = true;
        }
        else if (startsWithCommand(cursor, "KP"))
        {
            cursor += 2;
            if (!parseFixedMilli(cursor, message.value))
            {
                sendResponse("ERR bad KP value\r\n");
                reportSettingFailure();
                return;
            }
            message.cmd = SystemCommand::SetKp;
            parameter = ConfigParameter::Kp;
            hasParameter = true;
        }
        else if (startsWithCommand(cursor, "KI"))
        {
            cursor += 2;
            if (!parseFixedMilli(cursor, message.value))
            {
                sendResponse("ERR bad KI value\r\n");
                reportSettingFailure();
                return;
            }
            message.cmd = SystemCommand::SetKi;
            parameter = ConfigParameter::Ki;
            hasParameter = true;
        }
        else if (startsWithCommand(cursor, "KD"))
        {
            cursor += 2;
            if (!parseFixedMilli(cursor, message.value))
            {
                sendResponse("ERR bad KD value\r\n");
                reportSettingFailure();
                return;
            }
            message.cmd = SystemCommand::SetKd;
            parameter = ConfigParameter::Kd;
            hasParameter = true;
        }
        else if (startsWithCommand(cursor, "SAMPLE_TIME"))
        {
            cursor += 11;
            trimLeadingSpaces(cursor);
            if (!parseInteger(cursor, message.value))
            {
                sendResponse("ERR bad SAMPLE_TIME value\r\n");
                reportSettingFailure();
                return;
            }
            message.cmd = SystemCommand::SetSampleTime;
            parameter = ConfigParameter::SampleTime;
            hasParameter = true;
        }
        else
        {
            sendResponse("ERR unsupported SET command\r\n");
            reportSettingFailure();
            return;
        }

        if (hasParameter && !config_.isValid(parameter, message.value))
        {
            char response[64]{};
            std::snprintf(response, sizeof(response), "ERR invalid %s\r\n",
                          parameterName(parameter));
            sendResponse(response);
            reportSettingFailure();
            return;
        }
    }
    else if (isCommandLine(cursor, "STOP"))
    {
        message.cmd = SystemCommand::StopMotor;
    }
    else if (isCommandLine(cursor, "STATUS"))
    {
        message.cmd = SystemCommand::StatusRequest;
    }
    else if (startsWithCommand(cursor, "GET"))
    {
        cursor += 3;
        trimLeadingSpaces(cursor);
        if (isCommandLine(cursor, "RPM"))
        {
            sendParameterResponse(ConfigParameter::TargetRpm);
        }
        else if (isCommandLine(cursor, "KP"))
        {
            sendParameterResponse(ConfigParameter::Kp);
        }
        else if (isCommandLine(cursor, "KI"))
        {
            sendParameterResponse(ConfigParameter::Ki);
        }
        else if (isCommandLine(cursor, "KD"))
        {
            sendParameterResponse(ConfigParameter::Kd);
        }
        else if (isCommandLine(cursor, "SAMPLE_TIME"))
        {
            sendParameterResponse(ConfigParameter::SampleTime);
        }
        else
        {
            sendResponse("ERR unsupported GET command\r\n");
            reportSettingFailure();
            return;
        }
        reportSettingSucceeded();
        return;
    }
    else if (isCommandLine(cursor, "SAVE CONFIG"))
    {
        config_.save();
        sendResponse("OK\r\n");
        reportSettingSucceeded();
        return;
    }
    else if (isCommandLine(cursor, "LOAD CONFIG"))
    {
        config_.load();
        const RuntimeConfig &active = config_.active();
        message.cmd = SystemCommand::LoadConfig;
        message.value = active.targetRpm;
        message.aux = active.gains.kp;
        message.extra = active.gains.ki;
        message.detail = active.gains.kd;
        message.detail2 = static_cast<int32_t>(active.sampleTimeMs);
    }
    else if (isCommandLine(cursor, "RESET PID"))
    {
        message.cmd = SystemCommand::ResetPid;
    }
    else
    {
        sendResponse("ERR unsupported command\r\n");
        reportSettingFailure();
        return;
    }

    if (!sendToDirector(message))
    {
        sendResponse("ERR director busy\r\n");
        reportSettingFailure();
    }
}

void InterfaceManager::handleInterfaceEvent(const InterfaceEvent &event)
{
    switch (event.type)
    {
    case InterfaceEventType::CommandOk:
        applyConfirmedCommand(event);
        sendResponse("OK\r\n");
        break;

    case InterfaceEventType::ManagerBusy:
        sendResponse("ERR manager busy\r\n");
        break;

    case InterfaceEventType::Unsupported:
        sendResponse("ERR unsupported command\r\n");
        break;

    case InterfaceEventType::Status:
        sendStatusResponse(event.status);
        break;
    }
}

void InterfaceManager::applyConfirmedCommand(const InterfaceEvent &event)
{
    switch (event.command)
    {
    case SystemCommand::SetTargetRpm:
        (void)config_.set(ConfigParameter::TargetRpm, event.value);
        break;

    case SystemCommand::SetKp:
        (void)config_.set(ConfigParameter::Kp, event.value);
        break;

    case SystemCommand::SetKi:
        (void)config_.set(ConfigParameter::Ki, event.value);
        break;

    case SystemCommand::SetKd:
        (void)config_.set(ConfigParameter::Kd, event.value);
        break;

    case SystemCommand::SetSampleTime:
        (void)config_.set(ConfigParameter::SampleTime, event.value);
        break;

    case SystemCommand::StopMotor:
        (void)config_.set(ConfigParameter::TargetRpm, 0);
        break;

    case SystemCommand::LoadConfig:
        (void)config_.set(ConfigParameter::TargetRpm, event.value);
        (void)config_.set(ConfigParameter::Kp, event.aux);
        (void)config_.set(ConfigParameter::Ki, event.extra);
        (void)config_.set(ConfigParameter::Kd, event.detail);
        (void)config_.set(ConfigParameter::SampleTime, event.detail2);
        break;

    default:
        break;
    }
}

void InterfaceManager::sendParameterResponse(ConfigParameter parameter)
{
    char response[64]{};
    char gain[16]{};

    switch (parameter)
    {
    case ConfigParameter::Kp:
    case ConfigParameter::Ki:
    case ConfigParameter::Kd:
        formatGain(gain, sizeof(gain), config_.value(parameter));
        std::snprintf(response, sizeof(response), "%s %s\r\n", parameterName(parameter), gain);
        break;

    case ConfigParameter::TargetRpm:
    case ConfigParameter::SampleTime:
        std::snprintf(response, sizeof(response), "%s %ld\r\n", parameterName(parameter),
                      static_cast<long>(config_.value(parameter)));
        break;
    }

    sendResponse(response);
}

void InterfaceManager::sendStatusResponse(const SystemStatus &status)
{
    char kp[16]{};
    char ki[16]{};
    char kd[16]{};
    formatGain(kp, sizeof(kp), status.kp);
    formatGain(ki, sizeof(ki), status.ki);
    formatGain(kd, sizeof(kd), status.kd);

    char response[224]{};
    std::snprintf(response, sizeof(response),
                  "STATUS current_rpm=%ld target_rpm=%ld pwm_duty=%ld direction=%s "
                  "kp=%s ki=%s kd=%s pid_output=%ld encoder_count=%ld controller=%s\r\n",
                  static_cast<long>(status.currentRpm), static_cast<long>(status.targetRpm),
                  static_cast<long>(status.pwmDutyPermille), directionText(status.direction), kp,
                  ki, kd, static_cast<long>(status.pidOutput),
                  static_cast<long>(status.encoderCount),
                  status.controllerEnabled ? "ENABLED" : "STOPPED");
    sendResponse(response);
}

bool InterfaceManager::sendToDirector(const SystemMessage &message)
{
    return director_ != nullptr &&
           director_->sendEvent(message, config::kManagerQueueSendTimeoutMs);
}

void InterfaceManager::reportSettingSucceeded()
{
    (void)sendToDirector(
        SystemMessage{SystemCommand::SettingSucceeded, MessageSource::Interface, 0});
}

void InterfaceManager::reportSettingFailure()
{
    (void)sendToDirector(SystemMessage{SystemCommand::SettingFailed, MessageSource::Interface, 0});
}

} // namespace app

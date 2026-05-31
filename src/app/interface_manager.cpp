#include "app/interface_manager.hpp"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/ceo.hpp"
#include "app/ui_manager.hpp"

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

} // namespace

InterfaceManager::InterfaceManager(hal::IUart &uart) : uart_(uart)
{
}

void InterfaceManager::initialize(CEO &director, UIManager &blink)
{
    director_ = &director;
    blink_ = &blink;
    rxQueue_.create(config::kInterfaceRxQueueLength);
    resetPacket();
    uart_.initialize();
}

bool InterfaceManager::onRxByteFromIsr(uint8_t byte, void *higherPriorityTaskWoken)
{
    return rxQueue_.sendFromIsr(byte, static_cast<BaseType_t *>(higherPriorityTaskWoken));
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
    for (;;)
    {
        if (rxQueue_.receive(byte, portMAX_DELAY))
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
            if (blink_ != nullptr)
            {
                (void)blink_->sendEvent(BlinkEvent::ServiceMode,
                                        config::kManagerQueueSendTimeoutMs);
            }
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
        if (blink_ != nullptr)
        {
            (void)blink_->sendEvent(BlinkEvent::NormalMode, config::kManagerQueueSendTimeoutMs);
        }
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
    if (director_ == nullptr)
    {
        sendResponse("ERR director unavailable\r\n");
        reportSettingFailure();
        return;
    }

    const char *cursor = packet;
    SystemMessage message{SystemCommand::InvalidCommand, MessageSource::Interface, 0};

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
        }
        else
        {
            sendResponse("ERR unsupported SET command\r\n");
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
            message.cmd = SystemCommand::GetTargetRpm;
        }
        else if (isCommandLine(cursor, "KP"))
        {
            message.cmd = SystemCommand::GetKp;
        }
        else if (isCommandLine(cursor, "KI"))
        {
            message.cmd = SystemCommand::GetKi;
        }
        else if (isCommandLine(cursor, "KD"))
        {
            message.cmd = SystemCommand::GetKd;
        }
        else if (isCommandLine(cursor, "SAMPLE_TIME"))
        {
            message.cmd = SystemCommand::GetSampleTime;
        }
        else
        {
            sendResponse("ERR unsupported GET command\r\n");
            reportSettingFailure();
            return;
        }
    }
    else if (isCommandLine(cursor, "SAVE CONFIG"))
    {
        message.cmd = SystemCommand::SaveConfig;
    }
    else if (isCommandLine(cursor, "LOAD CONFIG"))
    {
        message.cmd = SystemCommand::LoadConfig;
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

    if (!director_->sendEvent(message, config::kManagerQueueSendTimeoutMs))
    {
        sendResponse("ERR director busy\r\n");
        reportSettingFailure();
    }
}

void InterfaceManager::reportSettingFailure()
{
    if (blink_ != nullptr)
    {
        (void)blink_->sendEvent(BlinkEvent::SettingFailed, config::kManagerQueueSendTimeoutMs);
    }
}

} // namespace app

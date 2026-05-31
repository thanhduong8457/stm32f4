#include "app/interface_manager.hpp"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/ui_manager.hpp"
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

    if (std::strncmp(cursor, "SET", 3) == 0 &&
        std::isspace(static_cast<unsigned char>(cursor[3])) != 0)
    {
        cursor += 3;
        trimLeadingSpaces(cursor);
        if (!parseInteger(cursor, message.value))
        {
            sendResponse("ERR bad SET value\r\n");
            reportSettingFailure();
            return;
        }
        message.cmd = SystemCommand::SetMotorTarget;
    }
    else if (isCommandLine(cursor, "STOP"))
    {
        message.cmd = SystemCommand::StopMotor;
    }
    else if (isCommandLine(cursor, "STATUS"))
    {
        message.cmd = SystemCommand::StatusRequest;
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

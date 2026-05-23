#include "app/interface_manager.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/director_manager.hpp"

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
    const long parsed = std::strtol(text, &end, 10);
    if (end == text)
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

} // namespace

InterfaceManager::InterfaceManager(hal::IUart &uart) : uart_(uart) {}

void InterfaceManager::initialize(DirectorManager &director)
{
    director_ = &director;
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
    if (director_ == nullptr)
    {
        sendResponse("ERR director unavailable\r\n");
        return;
    }

    const char *cursor = packet;
    trimLeadingSpaces(cursor);

    SystemMessage message{SystemCommand::InvalidCommand, MessageSource::Interface, 0};

    if (std::strncmp(cursor, "SET", 3) == 0 && std::isspace(static_cast<unsigned char>(cursor[3])) != 0)
    {
        cursor += 3;
        trimLeadingSpaces(cursor);
        if (!parseInteger(cursor, message.value))
        {
            sendResponse("ERR bad SET value\r\n");
            return;
        }
        message.cmd = SystemCommand::SetMotorTarget;
    }
    else if (std::strcmp(cursor, "STOP") == 0)
    {
        message.cmd = SystemCommand::StopMotor;
    }
    else if (std::strcmp(cursor, "STATUS") == 0)
    {
        message.cmd = SystemCommand::StatusRequest;
    }

    if (!director_->sendEvent(message, config::kManagerQueueSendTimeoutMs))
    {
        sendResponse("ERR director busy\r\n");
    }
}

} // namespace app

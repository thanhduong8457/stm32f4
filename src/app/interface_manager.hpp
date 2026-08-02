#pragma once

#include <cstdint>

#include "FreeRTOS.h"

#include "app/app_config.hpp"
#include "app/configuration_manager.hpp"
#include "app/system_messages.hpp"
#include "hal/uart.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

class CEO;
struct ParsedInterfaceCommand;

class InterfaceManager : public hal::IUartRxSink
{
public:
    explicit InterfaceManager(hal::IUart &uart);

    void initialize(CEO &director);
    void run();
    void processRx(char byte, hal::ByteStreamChannel channel = hal::ByteStreamChannel::Data);
    void sendResponse(const char *message);
    bool sendEvent(const InterfaceEvent &event, uint32_t timeoutMs = 0);
    bool onRxByteFromIsr(uint8_t byte, void *higherPriorityTaskWoken) override;
    bool onRxByteFromIsr(uint8_t byte, hal::ByteStreamChannel channel,
                         void *higherPriorityTaskWoken) override;

private:
    struct RxByte
    {
        uint8_t byte;
        hal::ByteStreamChannel channel;
    };

    void resetPacket();
    void dispatchPacket(const char *packet);
    void executeCommand(const ParsedInterfaceCommand &command);
    void handleInterfaceEvent(const InterfaceEvent &event);
    void applyConfirmedCommand(const InterfaceEvent &event);
    void sendParameterResponse(ConfigParameter parameter);
    void sendStatusResponse(const SystemStatus &status);
    bool sendToDirector(const SystemMessage &message);
    void reportSettingSucceeded();
    void reportSettingFailure();

    hal::IUart &uart_;
    CEO *director_ = nullptr;
    middleware::RtosQueue<RxByte> rxQueue_;
    middleware::RtosQueue<InterfaceEvent> eventQueue_;
    ConfigurationManager config_{};
    char packet_[config::kInterfacePacketMaxLength]{};
    uint8_t packetLength_ = 0;
    bool serviceMode_ = false;
    hal::ByteStreamChannel activeResponseChannel_ = hal::ByteStreamChannel::Data;
};

} // namespace app

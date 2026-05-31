#pragma once

#include <cstdint>

#include "FreeRTOS.h"

#include "app/app_config.hpp"
#include "app/system_messages.hpp"
#include "hal/uart.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

class UIManager;
class CEO;

class InterfaceManager : public hal::IUartRxSink
{
public:
    explicit InterfaceManager(hal::IUart &uart);

    void initialize(CEO &director, UIManager &blink);
    void run();
    void processRx(char byte);
    void sendResponse(const char *message);
    bool onRxByteFromIsr(uint8_t byte, void *higherPriorityTaskWoken) override;

private:
    void resetPacket();
    void dispatchPacket(const char *packet);
    void dispatchServiceCommand(const char *packet);
    void reportSettingFailure();

    hal::IUart &uart_;
    CEO *director_ = nullptr;
    UIManager *blink_ = nullptr;
    middleware::RtosQueue<uint8_t> rxQueue_;
    char packet_[config::kInterfacePacketMaxLength]{};
    uint8_t packetLength_ = 0;
    bool serviceMode_ = false;
};

} // namespace app

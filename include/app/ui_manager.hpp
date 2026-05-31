#pragma once

#include <cstdint>

#include "hal/digital_output.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

enum class BlinkEvent : uint8_t
{
    NormalMode = 1,
    ServiceMode,
    SettingSucceeded,
    SettingFailed,
};

class UIManager
{
public:
    explicit UIManager(hal::IDigitalOutput &led);

    void initialize();
    void run();
    bool sendEvent(BlinkEvent event, uint32_t timeoutMs = 0);

private:
    void handleEvent(BlinkEvent event, bool &serviceMode);
    void blinkStatus(uint8_t count);

    hal::IDigitalOutput &led_;
    middleware::RtosQueue<BlinkEvent> queue_;
};

} // namespace app

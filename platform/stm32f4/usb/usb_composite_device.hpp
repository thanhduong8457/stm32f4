#pragma once

#include <cstdint>

#include "hal/uart.hpp"

namespace platform::stm32f4
{

class UsbCompositeDevice final : public hal::IUart
{
public:
    void setRxSink(hal::IUartRxSink *sink) override;
    void initialize() override;
    void poll() override;
    void send(char ch) override;
    void send(const char *text) override;
    void sendTo(hal::ByteStreamChannel channel, char ch) override;
    void sendTo(hal::ByteStreamChannel channel, const char *text) override;
    bool sendKeyboardReport(uint8_t modifier, const uint8_t keycode[6]);
    bool releaseKeyboard();

private:
    void initializeHardware();
    void drainCdc(uint8_t cdcIndex, hal::ByteStreamChannel channel);
    uint8_t cdcIndexFor(hal::ByteStreamChannel channel) const;

    hal::IUartRxSink *rxSink_ = nullptr;
    bool initialized_ = false;
};

} // namespace platform::stm32f4

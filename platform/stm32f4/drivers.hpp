#pragma once

#include <cstdint>

#include "hal/digital_output.hpp"
#include "hal/encoder.hpp"
#include "hal/pwm_output.hpp"
#include "hal/uart.hpp"

namespace platform::stm32f4
{

class Uart1 final : public hal::IUart
{
public:
    void setRxSink(hal::IUartRxSink *sink) override;
    void initialize() override;
    void send(char ch) override;
    void send(const char *text) override;
    void handleIrq();

private:
    hal::IUartRxSink *rxSink_ = nullptr;
    void *txQueue_ = nullptr;
};

class Tim4Channel4Pwm final : public hal::IPwmOutput
{
public:
    void initialize() override;
    void setDutyCyclePermille(uint16_t dutyPermille) override;
};

class Tim3Encoder final : public hal::IEncoder
{
public:
    void initialize() override;
    int32_t read() const override;
};

class Pc13Led final : public hal::IDigitalOutput
{
public:
    void initialize() override;
    void set(bool active) override;
    bool get() const override;
};

} // namespace platform::stm32f4

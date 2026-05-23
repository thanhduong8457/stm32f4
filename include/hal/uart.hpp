#pragma once

#include <cstdint>

#include "hal/hardware_component.hpp"

namespace hal
{

class IUart : public IHardwareComponent
{
public:
    virtual void send(char ch) = 0;
    virtual void send(const char *text) = 0;
};

class IUartRxSink
{
public:
    virtual ~IUartRxSink() = default;
    virtual bool onRxByteFromIsr(uint8_t byte, void *higherPriorityTaskWoken) = 0;
};

} // namespace hal

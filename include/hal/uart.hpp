#pragma once

#include <cstdint>

#include "hal/hardware_component.hpp"

namespace hal
{

enum class ByteStreamChannel : uint8_t
{
    Data = 0,
    Service = 1,
};

class IUartRxSink
{
public:
    virtual ~IUartRxSink() = default;
    virtual bool onRxByteFromIsr(uint8_t byte, void *higherPriorityTaskWoken) = 0;
    virtual bool onRxByteFromIsr(uint8_t byte, ByteStreamChannel channel,
                                 void *higherPriorityTaskWoken)
    {
        (void)channel;
        return onRxByteFromIsr(byte, higherPriorityTaskWoken);
    }
};

class IUart : public IHardwareComponent
{
public:
    virtual void setRxSink(IUartRxSink *sink)
    {
        (void)sink;
    }

    virtual void poll()
    {
    }

    virtual void send(char ch) = 0;
    virtual void send(const char *text) = 0;

    virtual void sendTo(ByteStreamChannel channel, char ch)
    {
        (void)channel;
        send(ch);
    }

    virtual void sendTo(ByteStreamChannel channel, const char *text)
    {
        (void)channel;
        send(text);
    }
};

} // namespace hal

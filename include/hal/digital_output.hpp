#pragma once

#include "hal/hardware_component.hpp"

namespace hal
{

class IDigitalOutput : public IHardwareComponent
{
public:
    virtual void set(bool active) = 0;
    virtual bool get() const = 0;

    void toggle()
    {
        set(!get());
    }
};

} // namespace hal

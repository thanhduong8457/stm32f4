#pragma once

#include <cstdint>

#include "hal/hardware_component.hpp"

namespace hal
{

class IPwmOutput : public IHardwareComponent
{
public:
    virtual void setDutyCyclePermille(uint16_t dutyPermille) = 0;
};

} // namespace hal

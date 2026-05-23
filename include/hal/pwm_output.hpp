#pragma once

#include <cstdint>

#include "hal/hardware_component.hpp"

namespace hal
{

class IPwmOutput : public IHardwareComponent
{
public:
    virtual void setPulseWidthUs(uint16_t pulseWidthUs) = 0;
};

class IServoOutput : public IHardwareComponent
{
public:
    virtual void setAngleDegrees(uint8_t angleDegrees) = 0;
};

} // namespace hal

#pragma once

#include <cstdint>

#include "hal/hardware_component.hpp"

namespace hal
{

class IEncoder : public IHardwareComponent
{
public:
    virtual int32_t read() const = 0;
};

} // namespace hal

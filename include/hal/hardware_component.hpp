#pragma once

namespace hal
{

class IHardwareComponent
{
public:
    virtual ~IHardwareComponent() = default;
    virtual void initialize() = 0;
};

} // namespace hal

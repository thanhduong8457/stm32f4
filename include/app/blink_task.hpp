#pragma once

#include "hal/digital_output.hpp"

namespace app
{

class BlinkTask
{
public:
    explicit BlinkTask(hal::IDigitalOutput &led);

    void initialize();
    void run();

private:
    hal::IDigitalOutput &led_;
};

} // namespace app

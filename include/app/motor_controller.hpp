#pragma once

#include <cstdint>

#include "app/system_messages.hpp"
#include "hal/pwm_output.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

class MotorController
{
public:
    explicit MotorController(hal::IServoOutput &servo);

    void initialize();
    void run();
    bool sendCommand(const MotorCommand &command, uint32_t timeoutMs = 0);
    void setTarget(int32_t target);
    void updateOutput();

private:
    int32_t clampTarget(int32_t target) const;

    hal::IServoOutput &servo_;
    middleware::RtosQueue<MotorCommand> queue_;
    int32_t target_ = 0;
    int32_t appliedTarget_ = -1;
    bool enabled_ = false;
};

} // namespace app

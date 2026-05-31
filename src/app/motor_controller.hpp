#pragma once

#include <cstdint>

#include "app/system_messages.hpp"
#include "hal/pwm_output.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

/**
 * Owns the motor output task and translates motor commands into PWM duty-cycle
 * and enable-state updates.
 *
 * This class does not calculate speed-control output. It only applies duty
 * commands generated elsewhere and keeps hardware access local to the motor
 * manager.
 */
class MotorController
{
public:
    struct Status
    {
        int32_t dutyPermille = 0;
        bool enabled = false;
    };

    explicit MotorController(hal::IPwmOutput &pwm);

    void initialize();
    void run();
    bool sendCommand(const MotorCommand &command, uint32_t timeoutMs = 0);
    Status status() const;

private:
    void processCommand(const MotorCommand &command);
    void applyDuty(int32_t dutyPermille);

    hal::IPwmOutput &pwm_;
    middleware::RtosQueue<MotorCommand> queue_;
    Status status_{};
    int32_t appliedDutyPermille_ = -1;
};

} // namespace app

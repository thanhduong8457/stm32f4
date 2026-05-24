#pragma once

#include <cstdint>

#include "app/system_messages.hpp"
#include "hal/pwm_output.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

/**
 * Owns the motor output task and translates motor commands into servo updates.
 *
 * Other managers never call the servo driver directly. They enqueue
 * MotorCommand values here, and the motor task serializes command handling so
 * the PWM output is updated from one place.
 */
class MotorController
{
public:
    explicit MotorController(hal::IServoOutput &servo);

    // Create the command queue, initialize the platform PWM driver, and apply a
    // known startup output.
    void initialize();

    // Blocking task loop. This function is intended to be run by a FreeRTOS task
    // and never returns.
    void run();

    // Queue a command for the motor task. timeoutMs lets callers decide whether
    // to wait for queue space or fail immediately.
    bool sendCommand(const MotorCommand &command, uint32_t timeoutMs = 0);

    // Store a bounded target and mark output as enabled.
    void setTarget(int32_t target);

    // Push the requested target to the servo only when the effective output has
    // changed, keeping duplicate PWM writes and logs out of the hot path.
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

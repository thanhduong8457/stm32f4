#pragma once

#include <cstdint>

#include "app/pid_controller.hpp"
#include "app/system_messages.hpp"
#include "hal/pwm_output.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

/**
 * Owns the motor output task and translates speed-control commands into PWM
 * duty-cycle updates.
 *
 * Other managers never call the PWM driver directly. They enqueue MotorCommand
 * values here, and the motor task serializes command handling so the control
 * loop and PWM output are updated from one deterministic task.
 */
class MotorController
{
public:
    struct Status
    {
        int32_t targetRpm = 0;
        int32_t rampedTargetRpm = 0;
        int32_t actualRpm = 0;
        int32_t dutyPermille = 0;
        int32_t pidOutput = 0;
        bool enabled = false;
    };

    explicit MotorController(hal::IPwmOutput &pwm);

    // Create the command queue, initialize the platform PWM driver, and apply a
    // known startup output.
    void initialize();

    // Blocking task loop. This function is intended to be run by a FreeRTOS task
    // and never returns.
    void run();

    // Queue a command for the motor task. timeoutMs lets callers decide whether
    // to wait for queue space or fail immediately.
    bool sendCommand(const MotorCommand &command, uint32_t timeoutMs = 0);

    Status status() const;

private:
    void processCommand(const MotorCommand &command);
    void setTargetRpm(int32_t targetRpm);
    void stop();
    void runControlStep();
    void applyDuty(int32_t dutyPermille);
    void configurePid(PidGains gains);
    void setControlPeriod(uint32_t periodMs);
    int32_t nextRampedTarget();
    int32_t clampTargetRpm(int32_t targetRpm) const;

    hal::IPwmOutput &pwm_;
    middleware::RtosQueue<MotorCommand> queue_;
    PidController pid_{};
    Status status_{};
    PidGains gains_{};
    uint32_t controlPeriodMs_ = 20;
    bool enabled_ = false;
    int32_t targetRpm_ = 0;
    int32_t rampedTargetRpm_ = 0;
    int32_t actualRpm_ = 0;
    int32_t appliedDutyPermille_ = -1;
};

} // namespace app

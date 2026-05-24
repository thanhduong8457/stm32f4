#include "app/motor_controller.hpp"

#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"

namespace app
{

MotorController::MotorController(hal::IServoOutput &servo) : servo_(servo) {}

void MotorController::initialize()
{
    queue_.create(config::kMotorQueueLength);
    servo_.initialize();

    // Start from a deterministic zero target. setTarget enables the controller,
    // and updateOutput applies the initial angle to hardware.
    setTarget(0);
    updateOutput();
}

bool MotorController::sendCommand(const MotorCommand &command, uint32_t timeoutMs)
{
    return queue_.send(command, timeoutMs);
}

void MotorController::setTarget(int32_t target)
{
    // Clamp once at the boundary so the rest of the controller only works with
    // values the servo driver can safely accept.
    target_ = clampTarget(target);
    enabled_ = true;
}

void MotorController::updateOutput()
{
    // A stopped motor reports and applies zero, while preserving the regular
    // target path for enabled commands.
    const int32_t output = enabled_ ? target_ : 0;
    if (output == appliedTarget_)
    {
        return;
    }

    // Record before writing the servo so repeated calls with the same effective
    // target are cheap and do not spam UART logs.
    appliedTarget_ = output;
    servo_.setAngleDegrees(static_cast<uint8_t>(output));
    printf("[Motor] target=%ld\r\n", static_cast<long>(output));
}

void MotorController::run()
{
    MotorCommand command{};
    for (;;)
    {
        // The motor task sleeps here until another manager posts work. This
        // keeps command handling event-driven instead of polling.
        if (queue_.receive(command, portMAX_DELAY))
        {
            switch (command.cmd)
            {
            case SystemCommand::SetMotorTarget:
                setTarget(command.target);
                break;

            case SystemCommand::StopMotor:
                // Disable output immediately. The next SetMotorTarget command
                // re-enables the controller through setTarget().
                enabled_ = false;
                target_ = 0;
                break;

            default:
                // Ignore commands that are not part of the motor contract.
                break;
            }

            updateOutput();
        }
    }
}

int32_t MotorController::clampTarget(int32_t target) const
{
    if (target < config::kMotorTargetMin)
    {
        return config::kMotorTargetMin;
    }
    if (target > config::kMotorTargetMax)
    {
        return config::kMotorTargetMax;
    }
    return target;
}

} // namespace app

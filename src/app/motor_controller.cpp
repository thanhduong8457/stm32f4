#include "app/motor_controller.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"

namespace app
{

MotorController::MotorController(hal::IPwmOutput &pwm) : pwm_(pwm)
{
}

void MotorController::initialize()
{
    queue_.create(config::kMotorQueueLength);
    pwm_.initialize();
    applyDuty(0);
}

bool MotorController::sendCommand(const MotorCommand &command, uint32_t timeoutMs)
{
    return queue_.send(command, timeoutMs);
}

MotorController::Status MotorController::status() const
{
    taskENTER_CRITICAL();
    Status temp = status_;
    taskEXIT_CRITICAL();
    return temp;
}

void MotorController::run()
{
    MotorCommand command{};

    for (;;)
    {
        if (queue_.receive(command, portMAX_DELAY))
        {
            processCommand(command);
        }
    }
}

void MotorController::processCommand(const MotorCommand &command)
{
    switch (command.type)
    {
    case MotorCommandType::SetDuty:
        applyDuty(command.value);
        break;

    case MotorCommandType::Enable:
        status_.enabled = true;
        break;

    case MotorCommandType::Disable:
        status_.enabled = false;
        applyDuty(0);
        break;
    }
}

void MotorController::applyDuty(int32_t dutyPermille)
{
    if (dutyPermille < config::kPwmDutyMinPermille)
    {
        dutyPermille = config::kPwmDutyMinPermille;
    }
    if (dutyPermille > config::kPwmDutyMaxPermille)
    {
        dutyPermille = config::kPwmDutyMaxPermille;
    }

    status_.dutyPermille = dutyPermille;
    if (dutyPermille == appliedDutyPermille_)
    {
        return;
    }

    appliedDutyPermille_ = dutyPermille;
    pwm_.setDutyCyclePermille(static_cast<uint16_t>(dutyPermille));
}

} // namespace app

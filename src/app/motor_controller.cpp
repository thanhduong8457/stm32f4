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

    gains_.kp = config::kDefaultKp;
    gains_.ki = config::kDefaultKi;
    gains_.kd = config::kDefaultKd;
    pid_.configure(gains_);
    pid_.setOutputLimits(config::kPwmDutyMinPermille, config::kPwmDutyMaxPermille);
    setControlPeriod(config::kDefaultControlLoopPeriodMs);
    applyDuty(0);
}

bool MotorController::sendCommand(const MotorCommand &command, uint32_t timeoutMs)
{
    return queue_.send(command, timeoutMs);
}

MotorController::Status MotorController::status() const
{
    return status_;
}

void MotorController::run()
{
    TickType_t lastWake = xTaskGetTickCount();
    MotorCommand command{};

    for (;;)
    {
        while (queue_.receive(command, 0))
        {
            processCommand(command);
        }

        runControlStep();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(controlPeriodMs_));
    }
}

void MotorController::processCommand(const MotorCommand &command)
{
    switch (command.type)
    {
    case MotorCommandType::SetTargetRpm:
        setTargetRpm(command.value);
        break;

    case MotorCommandType::Stop:
        stop();
        break;

    case MotorCommandType::SetActualRpm:
        actualRpm_ = command.value < 0 ? -command.value : command.value;
        break;

    case MotorCommandType::SetKp:
        gains_.kp = command.value;
        configurePid(gains_);
        break;

    case MotorCommandType::SetKi:
        gains_.ki = command.value;
        configurePid(gains_);
        break;

    case MotorCommandType::SetKd:
        gains_.kd = command.value;
        configurePid(gains_);
        break;

    case MotorCommandType::SetControlPeriod:
        setControlPeriod(static_cast<uint32_t>(command.value));
        break;

    case MotorCommandType::ResetPid:
        pid_.reset();
        rampedTargetRpm_ = 0;
        break;
    }
}

void MotorController::setTargetRpm(int32_t targetRpm)
{
    targetRpm_ = clampTargetRpm(targetRpm);
    if (targetRpm_ == 0)
    {
        stop();
        return;
    }

    if (!enabled_)
    {
        pid_.reset();
        rampedTargetRpm_ = 0;
    }
    enabled_ = true;
}

void MotorController::stop()
{
    enabled_ = false;
    targetRpm_ = 0;
    rampedTargetRpm_ = 0;
    pid_.reset();
    applyDuty(0);
}

void MotorController::runControlStep()
{
    if (!enabled_)
    {
        applyDuty(0);
        status_.targetRpm = targetRpm_;
        status_.rampedTargetRpm = rampedTargetRpm_;
        status_.actualRpm = actualRpm_;
        status_.pidOutput = 0;
        status_.enabled = false;
        return;
    }

    const int32_t controlTarget = nextRampedTarget();
    const int32_t pidOutput = pid_.update(controlTarget, actualRpm_);
    applyDuty(pidOutput);

    status_.targetRpm = targetRpm_;
    status_.rampedTargetRpm = controlTarget;
    status_.actualRpm = actualRpm_;
    status_.pidOutput = pidOutput;
    status_.enabled = true;
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

void MotorController::configurePid(PidGains gains)
{
    pid_.configure(gains);
}

void MotorController::setControlPeriod(uint32_t periodMs)
{
    if (periodMs < config::kMinControlLoopPeriodMs)
    {
        periodMs = config::kMinControlLoopPeriodMs;
    }
    if (periodMs > config::kMaxControlLoopPeriodMs)
    {
        periodMs = config::kMaxControlLoopPeriodMs;
    }

    controlPeriodMs_ = periodMs;
    pid_.setControlPeriod(periodMs);
}

int32_t MotorController::nextRampedTarget()
{
    if (rampedTargetRpm_ >= targetRpm_)
    {
        rampedTargetRpm_ = targetRpm_;
        return rampedTargetRpm_;
    }

    int32_t step = static_cast<int32_t>(
        (static_cast<int64_t>(config::kSoftStartRampRpmPerSecond) * controlPeriodMs_) / 1000LL);
    if (step < 1)
    {
        step = 1;
    }

    rampedTargetRpm_ += step;
    if (rampedTargetRpm_ > targetRpm_)
    {
        rampedTargetRpm_ = targetRpm_;
    }
    return rampedTargetRpm_;
}

int32_t MotorController::clampTargetRpm(int32_t targetRpm) const
{
    if (targetRpm < config::kTargetRpmMin)
    {
        return config::kTargetRpmMin;
    }
    if (targetRpm > config::kTargetRpmMax)
    {
        return config::kTargetRpmMax;
    }
    return targetRpm;
}

} // namespace app

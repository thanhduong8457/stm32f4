#include "app/pid_manager.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/ceo.hpp"

namespace app
{

void PidManager::initialize(CEO &director)
{
    director_ = &director;
    queue_.create(config::kPidQueueLength);

    gains_.kp = config::kDefaultKp;
    gains_.ki = config::kDefaultKi;
    gains_.kd = config::kDefaultKd;
    pid_.configure(gains_);
    pid_.setOutputLimits(config::kPwmDutyMinPermille, config::kPwmDutyMaxPermille);
    status_.gains = gains_;
    setControlPeriod(config::kDefaultControlLoopPeriodMs);
    publishOutput(true);
}

bool PidManager::sendCommand(const PidCommand &command, uint32_t timeoutMs)
{
    return queue_.send(command, timeoutMs);
}

PidManager::Status PidManager::status() const
{
    taskENTER_CRITICAL();
    Status temp = status_;
    taskEXIT_CRITICAL();
    return temp;
}

void PidManager::run()
{
    TickType_t lastWake = xTaskGetTickCount();
    PidCommand command{};

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

void PidManager::processCommand(const PidCommand &command)
{
    switch (command.type)
    {
    case PidCommandType::SetTargetRpm:
        setTargetRpm(command.value);
        break;

    case PidCommandType::SetActualRpm:
        actualRpm_ = command.value < 0 ? -command.value : command.value;
        break;

    case PidCommandType::SetKp:
        gains_.kp = command.value;
        configurePid(gains_);
        break;

    case PidCommandType::SetKi:
        gains_.ki = command.value;
        configurePid(gains_);
        break;

    case PidCommandType::SetKd:
        gains_.kd = command.value;
        configurePid(gains_);
        break;

    case PidCommandType::SetControlPeriod:
        setControlPeriod(static_cast<uint32_t>(command.value));
        break;

    case PidCommandType::Reset:
        pid_.reset();
        rampedTargetRpm_ = 0;
        status_.rampedTargetRpm = 0;
        status_.outputPermille = 0;
        publishOutput(true);
        break;
    }
}

void PidManager::setTargetRpm(int32_t targetRpm)
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

void PidManager::stop()
{
    enabled_ = false;
    targetRpm_ = 0;
    rampedTargetRpm_ = 0;
    pid_.reset();
    status_.targetRpm = 0;
    status_.rampedTargetRpm = 0;
    status_.outputPermille = 0;
    status_.enabled = false;
    publishOutput(true);
}

void PidManager::runControlStep()
{
    status_.targetRpm = targetRpm_;
    status_.rampedTargetRpm = rampedTargetRpm_;
    status_.actualRpm = actualRpm_;
    status_.gains = gains_;
    status_.enabled = enabled_;

    if (!enabled_)
    {
        status_.outputPermille = 0;
        publishOutput();
        return;
    }

    const int32_t controlTarget = nextRampedTarget();
    const int32_t pidOutput = pid_.update(controlTarget, actualRpm_);

    status_.targetRpm = targetRpm_;
    status_.rampedTargetRpm = controlTarget;
    status_.actualRpm = actualRpm_;
    status_.outputPermille = pidOutput;
    status_.enabled = true;
    publishOutput();
}

void PidManager::configurePid(PidGains gains)
{
    pid_.configure(gains);
    status_.gains = gains;
}

void PidManager::setControlPeriod(uint32_t periodMs)
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

void PidManager::publishOutput(bool force)
{
    if (director_ == nullptr)
    {
        return;
    }

    if (!force && status_.outputPermille == lastPublishedDutyPermille_ &&
        status_.enabled == lastPublishedEnabled_)
    {
        return;
    }

    lastPublishedDutyPermille_ = status_.outputPermille;
    lastPublishedEnabled_ = status_.enabled;
    const SystemMessage message{SystemCommand::PidOutput, MessageSource::Pid,
                                status_.outputPermille, status_.targetRpm, status_.enabled ? 1 : 0};
    (void)director_->sendEvent(message, 0);
}

int32_t PidManager::nextRampedTarget()
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

int32_t PidManager::clampTargetRpm(int32_t targetRpm) const
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

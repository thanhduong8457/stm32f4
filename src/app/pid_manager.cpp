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
    pid_.configureFeedForward(config::kFeedForwardDutyAtMaxRpm, config::kTargetRpmMax);
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
        hasFeedback_ = true;
        feedbackFresh_ = true;
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
        controlState_.restart(targetRpm_ > 0, feedbackHealthy_, hasFeedback_);
        status_.rampedTargetRpm = 0;
        status_.outputPermille = 0;
        status_.controlState = controlState_.state();
        status_.enabled = controlState_.outputEnabled();
        publishOutput(true);
        break;
    }
}

void PidManager::setTargetRpm(int32_t targetRpm)
{
    const bool wasStopped = targetRpm_ == 0;
    targetRpm_ = clampTargetRpm(targetRpm);
    if (targetRpm_ == 0)
    {
        stop();
        return;
    }

    if (wasStopped)
    {
        pid_.reset();
        rampedTargetRpm_ = 0;
    }
    controlState_.updateInputs(true, feedbackHealthy_, hasFeedback_);
}

void PidManager::stop()
{
    targetRpm_ = 0;
    rampedTargetRpm_ = 0;
    controlState_.updateInputs(false, feedbackHealthy_, hasFeedback_);
    pid_.reset();
    status_.targetRpm = 0;
    status_.rampedTargetRpm = 0;
    status_.outputPermille = 0;
    status_.controlState = controlState_.state();
    status_.enabled = false;
    publishOutput(true);
}

void PidManager::runControlStep()
{
    updateFeedbackHealth();
    status_.targetRpm = targetRpm_;
    status_.rampedTargetRpm = rampedTargetRpm_;
    status_.actualRpm = actualRpm_;
    status_.gains = gains_;
    status_.controlState = controlState_.state();
    status_.enabled = controlState_.outputEnabled();
    status_.feedbackHealthy = feedbackHealthy_;

    if (!controlState_.outputEnabled())
    {
        status_.outputPermille = 0;
        publishOutput();
        return;
    }

    const int32_t controlTarget = nextRampedTarget();
    const int32_t pidOutput = pid_.update(controlTarget, actualRpm_);
    controlState_.setRampComplete(controlTarget == targetRpm_);

    status_.targetRpm = targetRpm_;
    status_.rampedTargetRpm = controlTarget;
    status_.actualRpm = actualRpm_;
    status_.outputPermille = pidOutput;
    status_.controlState = controlState_.state();
    status_.enabled = true;
    publishOutput();
}

void PidManager::updateFeedbackHealth()
{
    const bool wasHealthy = feedbackHealthy_;

    if (feedbackFresh_)
    {
        missedFeedbackPeriods_ = 0;
        feedbackHealthy_ = true;
    }
    else
    {
        if (missedFeedbackPeriods_ < config::kEncoderFeedbackTimeoutPeriods + 1U)
        {
            ++missedFeedbackPeriods_;
        }
        feedbackHealthy_ = hasFeedback_ &&
                           missedFeedbackPeriods_ <= config::kEncoderFeedbackTimeoutPeriods;
    }
    feedbackFresh_ = false;

    if (feedbackHealthy_ != wasHealthy)
    {
        // A fresh soft-start avoids applying stale integral or a duty step after
        // encoder feedback disappears and later recovers.
        pid_.reset();
        rampedTargetRpm_ = 0;
    }

    controlState_.updateInputs(targetRpm_ > 0, feedbackHealthy_, hasFeedback_);
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

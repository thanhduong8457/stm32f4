#include "app/pid_controller.hpp"

#include "app/app_config.hpp"

namespace app
{

void PidController::configure(PidGains gains)
{
    gains_ = gains;
    clampIntegral();
}

void PidController::setOutputLimits(int32_t minOutput, int32_t maxOutput)
{
    minOutput_ = minOutput;
    maxOutput_ = maxOutput;
    if (minOutput_ > maxOutput_)
    {
        const int32_t swap = minOutput_;
        minOutput_ = maxOutput_;
        maxOutput_ = swap;
    }
    output_ = clampOutput(output_);
    clampIntegral();
}

void PidController::setControlPeriod(uint32_t periodMs)
{
    periodMs_ = periodMs == 0U ? 1U : periodMs;
}

void PidController::reset()
{
    previousError_ = 0;
    integralErrorMs_ = 0;
    output_ = 0;
    hasPreviousError_ = false;
}

int32_t PidController::update(int32_t targetRpm, int32_t actualRpm)
{
    const int32_t error = targetRpm - actualRpm;
    const int64_t pTerm = (static_cast<int64_t>(gains_.kp) * error) / config::kPidGainScale;

    integralErrorMs_ += static_cast<int64_t>(error) * static_cast<int64_t>(periodMs_);
    clampIntegral();
    const int64_t iTerm =
        (static_cast<int64_t>(gains_.ki) * integralErrorMs_) / (config::kPidGainScale * 1000LL);

    int64_t dTerm = 0;
    if (hasPreviousError_)
    {
        const int32_t derivativeRpmPerSecond =
            static_cast<int32_t>(((static_cast<int64_t>(error) - previousError_) * 1000LL) /
                                 static_cast<int64_t>(periodMs_));
        dTerm = (static_cast<int64_t>(gains_.kd) * derivativeRpmPerSecond) / config::kPidGainScale;
    }

    previousError_ = error;
    hasPreviousError_ = true;
    output_ = clampOutput(pTerm + iTerm + dTerm);
    return output_;
}

int32_t PidController::output() const
{
    return output_;
}

PidGains PidController::gains() const
{
    return gains_;
}

int32_t PidController::clampOutput(int64_t value) const
{
    if (value < minOutput_)
    {
        return minOutput_;
    }
    if (value > maxOutput_)
    {
        return maxOutput_;
    }
    return static_cast<int32_t>(value);
}

void PidController::clampIntegral()
{
    if (gains_.ki == 0)
    {
        integralErrorMs_ = 0;
        return;
    }

    const int64_t positiveLimit =
        (static_cast<int64_t>(maxOutput_) * config::kPidGainScale * 1000LL) / gains_.ki;
    const int64_t negativeLimit =
        (static_cast<int64_t>(minOutput_) * config::kPidGainScale * 1000LL) / gains_.ki;

    if (integralErrorMs_ > positiveLimit)
    {
        integralErrorMs_ = positiveLimit;
    }
    else if (integralErrorMs_ < negativeLimit)
    {
        integralErrorMs_ = negativeLimit;
    }
}

} // namespace app

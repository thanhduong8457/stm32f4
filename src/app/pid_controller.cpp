#include "app/pid_controller.hpp"

#include "app/app_config.hpp"

namespace app
{

void PidController::configure(PidGains gains)
{
    gains_ = gains;
    clampIntegral();
}

void PidController::configureFeedForward(int32_t dutyAtMaxRpm, int32_t maxRpm)
{
    feedForwardDutyAtMaxRpm_ = dutyAtMaxRpm < 0 ? 0 : dutyAtMaxRpm;
    feedForwardMaxRpm_ = maxRpm > 0 ? maxRpm : 1;
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
    previousActualRpm_ = 0;
    integralErrorMs_ = 0;
    output_ = 0;
    filteredDTerm_ = 0;
    hasPreviousActualRpm_ = false;
}

int32_t PidController::update(int32_t targetRpm, int32_t actualRpm)
{
    const int32_t error = targetRpm - actualRpm;
    const int64_t feedForward = calculateFeedForward(targetRpm);
    const int64_t pTerm = (static_cast<int64_t>(gains_.kp) * error) / config::kPidGainScale;

    int64_t rawDTerm = 0;
    if (hasPreviousActualRpm_)
    {
        const int32_t rateOfChange =
            static_cast<int32_t>(((static_cast<int64_t>(actualRpm) - previousActualRpm_) * 1000LL) /
                                 static_cast<int64_t>(periodMs_));
        rawDTerm = -(static_cast<int64_t>(gains_.kd) * rateOfChange) / config::kPidGainScale;
    }

    if (hasPreviousActualRpm_)
    {
        filteredDTerm_ = (200LL * rawDTerm + 800LL * filteredDTerm_) / 1000LL;
    }
    else
    {
        filteredDTerm_ = rawDTerm;
    }

    previousActualRpm_ = actualRpm;
    hasPreviousActualRpm_ = true;

    const int64_t candidateIntegral = clampIntegralValue(
        integralErrorMs_ + static_cast<int64_t>(error) * static_cast<int64_t>(periodMs_));
    const int64_t candidateITerm =
        (static_cast<int64_t>(gains_.ki) * candidateIntegral) /
        (config::kPidGainScale * 1000LL);
    const int64_t candidateOutput = feedForward + pTerm + candidateITerm + filteredDTerm_;

    // Conditional integration lets the integral unwind at a limit, but prevents
    // it from accumulating farther in the direction of output saturation.
    const bool drivesOutOfHighSaturation = candidateOutput > maxOutput_ && error < 0;
    const bool drivesOutOfLowSaturation = candidateOutput < minOutput_ && error > 0;
    if ((candidateOutput >= minOutput_ && candidateOutput <= maxOutput_) ||
        drivesOutOfHighSaturation || drivesOutOfLowSaturation)
    {
        integralErrorMs_ = candidateIntegral;
    }

    const int64_t iTerm =
        (static_cast<int64_t>(gains_.ki) * integralErrorMs_) /
        (config::kPidGainScale * 1000LL);
    output_ = clampOutput(feedForward + pTerm + iTerm + filteredDTerm_);
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

int32_t PidController::calculateFeedForward(int32_t targetRpm) const
{
    if (targetRpm <= 0 || feedForwardDutyAtMaxRpm_ == 0)
    {
        return 0;
    }

    const int64_t duty =
        (static_cast<int64_t>(targetRpm) * feedForwardDutyAtMaxRpm_ +
         (feedForwardMaxRpm_ / 2)) /
        feedForwardMaxRpm_;
    return clampOutput(duty);
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
    integralErrorMs_ = clampIntegralValue(integralErrorMs_);
}

int64_t PidController::clampIntegralValue(int64_t value) const
{
    if (gains_.ki <= 0)
    {
        return 0;
    }

    const int64_t outputSpan = static_cast<int64_t>(maxOutput_) - minOutput_;
    const int64_t limit = (outputSpan * config::kPidGainScale * 1000LL) / gains_.ki;

    if (value > limit)
    {
        return limit;
    }
    if (value < -limit)
    {
        return -limit;
    }
    return value;
}

} // namespace app

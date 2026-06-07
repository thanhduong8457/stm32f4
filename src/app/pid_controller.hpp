#pragma once

#include <cstdint>

namespace app
{

struct PidGains
{
    int32_t kp = 0;
    int32_t ki = 0;
    int32_t kd = 0;
};

class PidController
{
public:
    void configure(PidGains gains);
    void setOutputLimits(int32_t minOutput, int32_t maxOutput);
    void setControlPeriod(uint32_t periodMs);
    void reset();

    int32_t update(int32_t targetRpm, int32_t actualRpm);
    int32_t output() const;
    PidGains gains() const;

private:
    int32_t clampOutput(int64_t value) const;
    void clampIntegral();

    PidGains gains_{};
    uint32_t periodMs_ = 20;
    int32_t minOutput_ = 0;
    int32_t maxOutput_ = 1000;
    int32_t previousActualRpm_ = 0;
    int64_t integralErrorMs_ = 0;
    int32_t output_ = 0;
    int64_t filteredDTerm_ = 0;
    bool hasPreviousActualRpm_ = false;
};

} // namespace app

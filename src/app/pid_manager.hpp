#pragma once

#include <cstdint>

#include "app/pid_controller.hpp"
#include "app/system_messages.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

class CEO;

class PidManager
{
public:
    struct Status
    {
        int32_t targetRpm = 0;
        int32_t rampedTargetRpm = 0;
        int32_t actualRpm = 0;
        int32_t outputPermille = 0;
        PidGains gains{};
        bool enabled = false;
    };

    PidManager() = default;

    void initialize(CEO &director);
    void run();
    bool sendCommand(const PidCommand &command, uint32_t timeoutMs = 0);
    Status status() const;

private:
    void processCommand(const PidCommand &command);
    void setTargetRpm(int32_t targetRpm);
    void stop();
    void runControlStep();
    void configurePid(PidGains gains);
    void setControlPeriod(uint32_t periodMs);
    void publishOutput(bool force = false);
    int32_t nextRampedTarget();
    int32_t clampTargetRpm(int32_t targetRpm) const;

    CEO *director_ = nullptr;
    middleware::RtosQueue<PidCommand> queue_;
    PidController pid_{};
    Status status_{};
    PidGains gains_{};
    uint32_t controlPeriodMs_ = 20;
    bool enabled_ = false;
    int32_t targetRpm_ = 0;
    int32_t rampedTargetRpm_ = 0;
    int32_t actualRpm_ = 0;
    int32_t lastPublishedDutyPermille_ = -1;
    bool lastPublishedEnabled_ = false;
};

} // namespace app

#pragma once

#include <cstdint>

#include "app/pid_controller.hpp"

namespace app
{

enum class ConfigParameter : uint8_t
{
    TargetRpm,
    Kp,
    Ki,
    Kd,
    SampleTime,
};

struct RuntimeConfig
{
    int32_t targetRpm = 0;
    PidGains gains{};
    uint32_t sampleTimeMs = 20;
};

class ConfigurationManager
{
public:
    ConfigurationManager();

    const RuntimeConfig &active() const;
    const RuntimeConfig &saved() const;

    bool set(ConfigParameter parameter, int32_t value);
    bool isValid(ConfigParameter parameter, int32_t value) const;
    int32_t value(ConfigParameter parameter) const;

    void save();
    void load();
    void resetToDefaults();

private:
    RuntimeConfig defaults() const;

    RuntimeConfig active_{};
    RuntimeConfig saved_{};
};

} // namespace app

#include "app/configuration_manager.hpp"

#include "app/app_config.hpp"

namespace app
{

ConfigurationManager::ConfigurationManager()
{
    active_ = defaults();
    saved_ = active_;
}

const RuntimeConfig &ConfigurationManager::active() const
{
    return active_;
}

const RuntimeConfig &ConfigurationManager::saved() const
{
    return saved_;
}

bool ConfigurationManager::set(ConfigParameter parameter, int32_t value)
{
    if (!isValid(parameter, value))
    {
        return false;
    }

    switch (parameter)
    {
    case ConfigParameter::TargetRpm:
        active_.targetRpm = value;
        break;

    case ConfigParameter::Kp:
        active_.gains.kp = value;
        break;

    case ConfigParameter::Ki:
        active_.gains.ki = value;
        break;

    case ConfigParameter::Kd:
        active_.gains.kd = value;
        break;

    case ConfigParameter::SampleTime:
        active_.sampleTimeMs = static_cast<uint32_t>(value);
        break;
    }

    return true;
}

bool ConfigurationManager::isValid(ConfigParameter parameter, int32_t value) const
{
    switch (parameter)
    {
    case ConfigParameter::TargetRpm:
        return value >= config::kTargetRpmMin && value <= config::kTargetRpmMax;

    case ConfigParameter::Kp:
    case ConfigParameter::Ki:
    case ConfigParameter::Kd:
        return value >= config::kPidGainMin && value <= config::kPidGainMax;

    case ConfigParameter::SampleTime:
        return value >= static_cast<int32_t>(config::kMinControlLoopPeriodMs) &&
               value <= static_cast<int32_t>(config::kMaxControlLoopPeriodMs);
    }

    return false;
}

int32_t ConfigurationManager::value(ConfigParameter parameter) const
{
    switch (parameter)
    {
    case ConfigParameter::TargetRpm:
        return active_.targetRpm;

    case ConfigParameter::Kp:
        return active_.gains.kp;

    case ConfigParameter::Ki:
        return active_.gains.ki;

    case ConfigParameter::Kd:
        return active_.gains.kd;

    case ConfigParameter::SampleTime:
        return static_cast<int32_t>(active_.sampleTimeMs);
    }

    return 0;
}

void ConfigurationManager::save()
{
    saved_ = active_;
}

void ConfigurationManager::load()
{
    active_ = saved_;
}

void ConfigurationManager::resetToDefaults()
{
    active_ = defaults();
}

RuntimeConfig ConfigurationManager::defaults() const
{
    RuntimeConfig runtime{};
    runtime.targetRpm = config::kDefaultTargetRpm;
    runtime.gains.kp = config::kDefaultKp;
    runtime.gains.ki = config::kDefaultKi;
    runtime.gains.kd = config::kDefaultKd;
    runtime.sampleTimeMs = config::kDefaultControlLoopPeriodMs;
    return runtime;
}

} // namespace app

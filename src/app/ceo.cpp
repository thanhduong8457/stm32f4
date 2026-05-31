#include "app/ceo.hpp"

#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/encoder_manager.hpp"
#include "app/interface_manager.hpp"
#include "app/motor_controller.hpp"
#include "app/ui_manager.hpp"

namespace app
{
namespace
{

const char *directionText(RotationDirection direction)
{
    switch (direction)
    {
    case RotationDirection::Cw:
        return "CW";

    case RotationDirection::Ccw:
        return "CCW";

    case RotationDirection::Stopped:
    default:
        return "STOPPED";
    }
}

void formatGain(char *buffer, size_t size, int32_t gain)
{
    const int32_t whole = gain / config::kPidGainScale;
    const int32_t fraction = gain % config::kPidGainScale;
    std::snprintf(buffer, size, "%ld.%03ld", static_cast<long>(whole), static_cast<long>(fraction));
}

const char *parameterName(ConfigParameter parameter)
{
    switch (parameter)
    {
    case ConfigParameter::TargetRpm:
        return "RPM";

    case ConfigParameter::Kp:
        return "KP";

    case ConfigParameter::Ki:
        return "KI";

    case ConfigParameter::Kd:
        return "KD";

    case ConfigParameter::SampleTime:
        return "SAMPLE_TIME";
    }

    return "UNKNOWN";
}

} // namespace

CEO::CEO(InterfaceManager &interface, MotorController &motor) : interface_(interface), motor_(motor)
{
}

void CEO::initialize(UIManager &blink, EncoderManager &encoder)
{
    blink_ = &blink;
    encoder_ = &encoder;
    queue_.create(config::kDirectorQueueLength);
}

bool CEO::sendEvent(const SystemMessage &message, uint32_t timeoutMs)
{
    return queue_.send(message, timeoutMs);
}

void CEO::processCommand(const SystemMessage &message)
{
    switch (message.cmd)
    {
    case SystemCommand::SetTargetRpm:
        reportSettingResult(applyConfigParameter(ConfigParameter::TargetRpm, message.value,
                                                 MotorCommandType::SetTargetRpm));
        break;

    case SystemCommand::SetKp:
        reportSettingResult(
            applyConfigParameter(ConfigParameter::Kp, message.value, MotorCommandType::SetKp));
        break;

    case SystemCommand::SetKi:
        reportSettingResult(
            applyConfigParameter(ConfigParameter::Ki, message.value, MotorCommandType::SetKi));
        break;

    case SystemCommand::SetKd:
        reportSettingResult(
            applyConfigParameter(ConfigParameter::Kd, message.value, MotorCommandType::SetKd));
        break;

    case SystemCommand::SetSampleTime:
        if (!config_.isValid(ConfigParameter::SampleTime, message.value))
        {
            interface_.sendResponse("ERR SAMPLE_TIME range 5..1000\r\n");
            reportSettingResult(false);
            return;
        }
        if (!sendMotorCommand(MotorCommandType::SetControlPeriod, message.value) ||
            !sendEncoderCommand(EncoderCommandType::SetSamplePeriod, message.value))
        {
            interface_.sendResponse("ERR manager busy\r\n");
            reportSettingResult(false);
            return;
        }
        (void)config_.set(ConfigParameter::SampleTime, message.value);
        interface_.sendResponse("OK\r\n");
        reportSettingResult(true);
        break;

    case SystemCommand::StopMotor:
        if (!sendMotorCommand(MotorCommandType::Stop, 0))
        {
            interface_.sendResponse("ERR motor busy\r\n");
            reportSettingResult(false);
            return;
        }
        (void)config_.set(ConfigParameter::TargetRpm, 0);
        interface_.sendResponse("OK\r\n");
        reportSettingResult(true);
        break;

    case SystemCommand::EncoderFeedback:
        state_.encoderDelta = message.value - state_.encoderCount;
        state_.encoderCount = message.value;
        state_.currentRpm = message.aux;
        state_.direction = static_cast<RotationDirection>(message.extra);
        (void)sendMotorCommand(MotorCommandType::SetActualRpm, state_.currentRpm);
        break;

    case SystemCommand::StatusRequest:
        sendStatusResponse();
        reportSettingResult(true);
        break;

    case SystemCommand::GetTargetRpm:
        sendGetResponse(ConfigParameter::TargetRpm);
        reportSettingResult(true);
        break;

    case SystemCommand::GetKp:
        sendGetResponse(ConfigParameter::Kp);
        reportSettingResult(true);
        break;

    case SystemCommand::GetKi:
        sendGetResponse(ConfigParameter::Ki);
        reportSettingResult(true);
        break;

    case SystemCommand::GetKd:
        sendGetResponse(ConfigParameter::Kd);
        reportSettingResult(true);
        break;

    case SystemCommand::GetSampleTime:
        sendGetResponse(ConfigParameter::SampleTime);
        reportSettingResult(true);
        break;

    case SystemCommand::SaveConfig:
        config_.save();
        interface_.sendResponse("OK\r\n");
        reportSettingResult(true);
        break;

    case SystemCommand::LoadConfig:
        config_.load();
        if (!applyActiveConfig())
        {
            interface_.sendResponse("ERR manager busy\r\n");
            reportSettingResult(false);
            return;
        }
        interface_.sendResponse("OK\r\n");
        reportSettingResult(true);
        break;

    case SystemCommand::ResetPid:
        if (!sendMotorCommand(MotorCommandType::ResetPid, 0))
        {
            interface_.sendResponse("ERR motor busy\r\n");
            reportSettingResult(false);
            return;
        }
        interface_.sendResponse("OK\r\n");
        reportSettingResult(true);
        break;

    case SystemCommand::InvalidCommand:
    default:
        interface_.sendResponse("ERR unsupported command\r\n");
        reportSettingResult(false);
        break;
    }
}

void CEO::run()
{
    SystemMessage message{};
    for (;;)
    {
        if (queue_.receive(message, portMAX_DELAY))
        {
            processCommand(message);
        }
    }
}

const CEO::State &CEO::state() const
{
    return state_;
}

bool CEO::applyConfigParameter(ConfigParameter parameter, int32_t value,
                               MotorCommandType motorCommand)
{
    if (!config_.isValid(parameter, value))
    {
        char response[64]{};
        std::snprintf(response, sizeof(response), "ERR invalid %s\r\n", parameterName(parameter));
        interface_.sendResponse(response);
        return false;
    }

    if (!sendMotorCommand(motorCommand, value))
    {
        interface_.sendResponse("ERR motor busy\r\n");
        return false;
    }

    (void)config_.set(parameter, value);
    interface_.sendResponse("OK\r\n");
    return true;
}

bool CEO::applyActiveConfig()
{
    const RuntimeConfig &active = config_.active();
    return sendMotorCommand(MotorCommandType::SetKp, active.gains.kp) &&
           sendMotorCommand(MotorCommandType::SetKi, active.gains.ki) &&
           sendMotorCommand(MotorCommandType::SetKd, active.gains.kd) &&
           sendMotorCommand(MotorCommandType::SetControlPeriod,
                            static_cast<int32_t>(active.sampleTimeMs)) &&
           sendEncoderCommand(EncoderCommandType::SetSamplePeriod,
                              static_cast<int32_t>(active.sampleTimeMs)) &&
           sendMotorCommand(MotorCommandType::SetTargetRpm, active.targetRpm);
}

bool CEO::sendMotorCommand(MotorCommandType type, int32_t value)
{
    const MotorCommand motorCommand{type, value};
    return motor_.sendCommand(motorCommand, config::kManagerQueueSendTimeoutMs);
}

bool CEO::sendEncoderCommand(EncoderCommandType type, int32_t value)
{
    if (encoder_ == nullptr)
    {
        return false;
    }

    const EncoderCommand command{type, value};
    return encoder_->sendCommand(command, config::kManagerQueueSendTimeoutMs);
}

void CEO::sendGetResponse(ConfigParameter parameter)
{
    char response[64]{};
    char gain[16]{};

    switch (parameter)
    {
    case ConfigParameter::Kp:
    case ConfigParameter::Ki:
    case ConfigParameter::Kd:
        formatGain(gain, sizeof(gain), config_.value(parameter));
        std::snprintf(response, sizeof(response), "%s %s\r\n", parameterName(parameter), gain);
        break;

    case ConfigParameter::TargetRpm:
    case ConfigParameter::SampleTime:
        std::snprintf(response, sizeof(response), "%s %ld\r\n", parameterName(parameter),
                      static_cast<long>(config_.value(parameter)));
        break;
    }

    interface_.sendResponse(response);
}

void CEO::sendStatusResponse()
{
    syncMotorStatus();

    char kp[16]{};
    char ki[16]{};
    char kd[16]{};
    formatGain(kp, sizeof(kp), config_.active().gains.kp);
    formatGain(ki, sizeof(ki), config_.active().gains.ki);
    formatGain(kd, sizeof(kd), config_.active().gains.kd);

    char response[224]{};
    std::snprintf(response, sizeof(response),
                  "STATUS current_rpm=%ld target_rpm=%ld pwm_duty=%ld direction=%s "
                  "kp=%s ki=%s kd=%s pid_output=%ld encoder_count=%ld controller=%s\r\n",
                  static_cast<long>(state_.currentRpm), static_cast<long>(state_.targetRpm),
                  static_cast<long>(state_.pwmDutyPermille), directionText(state_.direction), kp,
                  ki, kd, static_cast<long>(state_.pidOutput),
                  static_cast<long>(state_.encoderCount),
                  state_.controllerEnabled ? "ENABLED" : "STOPPED");
    interface_.sendResponse(response);
}

void CEO::syncMotorStatus()
{
    const MotorController::Status motorStatus = motor_.status();
    state_.targetRpm = motorStatus.targetRpm;
    state_.currentRpm = motorStatus.actualRpm;
    state_.pwmDutyPermille = motorStatus.dutyPermille;
    state_.pidOutput = motorStatus.pidOutput;
    state_.controllerEnabled = motorStatus.enabled;
}

void CEO::reportSettingResult(bool succeeded)
{
    if (blink_ != nullptr)
    {
        const BlinkEvent event =
            succeeded ? BlinkEvent::SettingSucceeded : BlinkEvent::SettingFailed;
        (void)blink_->sendEvent(event, config::kManagerQueueSendTimeoutMs);
    }
}

} // namespace app

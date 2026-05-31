#include "app/ceo.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/encoder_manager.hpp"
#include "app/interface_manager.hpp"
#include "app/motor_controller.hpp"
#include "app/pid_manager.hpp"
#include "app/ui_manager.hpp"

namespace app
{

CEO::CEO(InterfaceManager &interface, MotorController &motor, PidManager &pid)
    : interface_(interface), motor_(motor), pid_(pid)
{
}

void CEO::initialize(UIManager &ui, EncoderManager &encoder)
{
    ui_ = &ui;
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
    case SystemCommand::ServiceModeEntered:
        if (ui_ != nullptr)
        {
            (void)ui_->sendEvent(BlinkEvent::ServiceMode, config::kManagerQueueSendTimeoutMs);
        }
        break;

    case SystemCommand::ServiceModeExited:
        if (ui_ != nullptr)
        {
            (void)ui_->sendEvent(BlinkEvent::NormalMode, config::kManagerQueueSendTimeoutMs);
        }
        break;

    case SystemCommand::SettingSucceeded:
        reportSettingResult(true);
        break;

    case SystemCommand::SettingFailed:
        reportSettingResult(false);
        break;

    case SystemCommand::SetTargetRpm:
        reportCommandResult(routeToPid(PidCommandType::SetTargetRpm, message.value), message);
        break;

    case SystemCommand::SetKp:
        reportCommandResult(routeToPid(PidCommandType::SetKp, message.value), message);
        break;

    case SystemCommand::SetKi:
        reportCommandResult(routeToPid(PidCommandType::SetKi, message.value), message);
        break;

    case SystemCommand::SetKd:
        reportCommandResult(routeToPid(PidCommandType::SetKd, message.value), message);
        break;

    case SystemCommand::SetSampleTime:
        reportCommandResult(routeToPid(PidCommandType::SetControlPeriod, message.value) &&
                                routeToEncoder(EncoderCommandType::SetSamplePeriod, message.value),
                            message);
        break;

    case SystemCommand::StopMotor:
        reportCommandResult(routeToPid(PidCommandType::SetTargetRpm, 0) &&
                                routeToMotor(MotorCommandType::Disable, 0),
                            message);
        break;

    case SystemCommand::LoadConfig:
        reportCommandResult(
            routeToPid(PidCommandType::SetTargetRpm, message.value) &&
                routeToPid(PidCommandType::SetKp, message.aux) &&
                routeToPid(PidCommandType::SetKi, message.extra) &&
                routeToPid(PidCommandType::SetKd, message.detail) &&
                routeToPid(PidCommandType::SetControlPeriod, message.detail2) &&
                routeToEncoder(EncoderCommandType::SetSamplePeriod, message.detail2),
            message);
        break;

    case SystemCommand::ResetPid:
        reportCommandResult(routeToPid(PidCommandType::Reset, 0), message);
        break;

    case SystemCommand::EncoderFeedback:
        state_.encoderCount = message.value;
        state_.encoderDelta = message.detail;
        state_.currentRpm = message.aux;
        state_.direction = static_cast<RotationDirection>(message.extra);
        (void)routeToPid(PidCommandType::SetActualRpm, state_.currentRpm);
        break;

    case SystemCommand::PidOutput:
        if (message.extra != 0)
        {
            (void)routeToMotor(MotorCommandType::Enable, 0);
            (void)routeToMotor(MotorCommandType::SetDuty, message.value);
        }
        else
        {
            (void)routeToMotor(MotorCommandType::Disable, 0);
        }
        state_.pidOutput = message.value;
        state_.targetRpm = message.aux;
        state_.controllerEnabled = message.extra != 0;
        break;

    case SystemCommand::StatusRequest:
        sendStatus();
        reportSettingResult(true);
        break;

    case SystemCommand::InvalidCommand:
    default:
        sendInterfaceEvent(InterfaceEvent{InterfaceEventType::Unsupported, message.cmd});
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

bool CEO::routeToPid(PidCommandType type, int32_t value)
{
    return pid_.sendCommand(PidCommand{type, value}, config::kManagerQueueSendTimeoutMs);
}

bool CEO::routeToMotor(MotorCommandType type, int32_t value)
{
    return motor_.sendCommand(MotorCommand{type, value}, config::kManagerQueueSendTimeoutMs);
}

bool CEO::routeToEncoder(EncoderCommandType type, int32_t value)
{
    if (encoder_ == nullptr)
    {
        return false;
    }
    return encoder_->sendCommand(EncoderCommand{type, value}, config::kManagerQueueSendTimeoutMs);
}

void CEO::sendInterfaceEvent(const InterfaceEvent &event)
{
    (void)interface_.sendEvent(event, config::kManagerQueueSendTimeoutMs);
}

void CEO::reportCommandResult(bool succeeded, const SystemMessage &request)
{
    InterfaceEvent event{succeeded ? InterfaceEventType::CommandOk
                                   : InterfaceEventType::ManagerBusy,
                         request.cmd,
                         request.value,
                         request.aux,
                         request.extra,
                         request.detail,
                         request.detail2};
    sendInterfaceEvent(event);
    reportSettingResult(succeeded);
}

void CEO::reportSettingResult(bool succeeded)
{
    if (ui_ == nullptr)
    {
        return;
    }

    const BlinkEvent event = succeeded ? BlinkEvent::SettingSucceeded : BlinkEvent::SettingFailed;
    (void)ui_->sendEvent(event, config::kManagerQueueSendTimeoutMs);
}

void CEO::sendStatus()
{
    syncState();
    InterfaceEvent event{InterfaceEventType::Status};
    event.status.currentRpm = state_.currentRpm;
    event.status.targetRpm = state_.targetRpm;
    event.status.pwmDutyPermille = state_.pwmDutyPermille;
    event.status.direction = state_.direction;
    event.status.pidOutput = state_.pidOutput;
    event.status.encoderCount = state_.encoderCount;
    event.status.controllerEnabled = state_.controllerEnabled;

    const PidManager::Status pidStatus = pid_.status();
    event.status.kp = pidStatus.gains.kp;
    event.status.ki = pidStatus.gains.ki;
    event.status.kd = pidStatus.gains.kd;
    sendInterfaceEvent(event);
}

void CEO::syncState()
{
    const MotorController::Status motorStatus = motor_.status();
    const PidManager::Status pidStatus = pid_.status();

    state_.currentRpm = pidStatus.actualRpm;
    state_.targetRpm = pidStatus.targetRpm;
    state_.pwmDutyPermille = motorStatus.dutyPermille;
    state_.pidOutput = pidStatus.outputPermille;
    state_.controllerEnabled = pidStatus.enabled && motorStatus.enabled;

    if (encoder_ != nullptr)
    {
        state_.encoderCount = encoder_->count();
        state_.currentRpm = encoder_->rpm();
        state_.direction = encoder_->direction();
    }
}

} // namespace app

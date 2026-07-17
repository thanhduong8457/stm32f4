#include "app/interface_manager.hpp"

#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/ceo.hpp"
#include "app/interface_command_parser.hpp"

namespace app
{
namespace
{

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

const char *controlStateText(SpeedControlState state)
{
    switch (state)
    {
    case SpeedControlState::AwaitingFeedback:
        return "WAIT_FEEDBACK";

    case SpeedControlState::SoftStart:
        return "SOFT_START";

    case SpeedControlState::ClosedLoop:
        return "CLOSED_LOOP";

    case SpeedControlState::FeedbackFault:
        return "FEEDBACK_FAULT";

    case SpeedControlState::Stopped:
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

} // namespace

InterfaceManager::InterfaceManager(hal::IUart &uart) : uart_(uart)
{
}

void InterfaceManager::initialize(CEO &director)
{
    director_ = &director;
    uart_.setRxSink(this);
    rxQueue_.create(config::kInterfaceRxQueueLength);
    eventQueue_.create(config::kInterfaceEventQueueLength);
    resetPacket();
    uart_.initialize();
}

bool InterfaceManager::onRxByteFromIsr(uint8_t byte, void *higherPriorityTaskWoken)
{
    return onRxByteFromIsr(byte, hal::ByteStreamChannel::Data, higherPriorityTaskWoken);
}

bool InterfaceManager::onRxByteFromIsr(uint8_t byte, hal::ByteStreamChannel channel,
                                       void *higherPriorityTaskWoken)
{
    RxByte item{byte, channel};

    if (higherPriorityTaskWoken == nullptr)
    {
        return rxQueue_.send(item, 0);
    }

    return rxQueue_.sendFromIsr(item, static_cast<BaseType_t *>(higherPriorityTaskWoken));
}

bool InterfaceManager::sendEvent(const InterfaceEvent &event, uint32_t timeoutMs)
{
    return eventQueue_.send(event, timeoutMs);
}

void InterfaceManager::sendResponse(const char *message)
{
    uart_.sendTo(activeResponseChannel_, message);
}

void InterfaceManager::processRx(char byte, hal::ByteStreamChannel channel)
{
    activeResponseChannel_ = channel;

    if (byte == '\b' || byte == 0x7F)
    {
        if (packetLength_ > 0U)
        {
            --packetLength_;
            packet_[packetLength_] = '\0';
            sendResponse("\b \b");
        }
        return;
    }

    uart_.sendTo(channel, byte);

    if (byte == '\r' || byte == '\n')
    {
        if (packetLength_ > 0U)
        {
            packet_[packetLength_] = '\0';
            dispatchPacket(packet_);
            resetPacket();
        }
        return;
    }

    if (packetLength_ >= (config::kInterfacePacketMaxLength - 1U))
    {
        resetPacket();
        sendResponse("ERR packet too long\r\n");
        if (serviceMode_)
        {
            reportSettingFailure();
        }
        return;
    }

    packet_[packetLength_++] = byte;
}

void InterfaceManager::run()
{
    RxByte byte{};
    InterfaceEvent event{};

    for (;;)
    {
        uart_.poll();

        while (eventQueue_.receive(event, 0))
        {
            handleInterfaceEvent(event);
        }

        if (rxQueue_.receive(byte, pdMS_TO_TICKS(10)))
        {
            processRx(static_cast<char>(byte.byte), byte.channel);
        }
    }
}

void InterfaceManager::resetPacket()
{
    packetLength_ = 0;
    packet_[0] = '\0';
}

void InterfaceManager::dispatchPacket(const char *packet)
{
    executeCommand(parseInterfaceCommand(packet, serviceMode_));
}

void InterfaceManager::executeCommand(const ParsedInterfaceCommand &command)
{
    switch (command.action)
    {
    case InterfaceCommandAction::EnterServiceMode:
        serviceMode_ = true;
        (void)sendToDirector(
            SystemMessage{SystemCommand::ServiceModeEntered, MessageSource::Interface, 0});
        sendResponse("OK service mode entered\r\n");
        return;

    case InterfaceCommandAction::ExitServiceMode:
        serviceMode_ = false;
        (void)sendToDirector(
            SystemMessage{SystemCommand::ServiceModeExited, MessageSource::Interface, 0});
        sendResponse("OK service mode exited\r\n");
        return;

    case InterfaceCommandAction::ServiceModeAlreadyActive:
        sendResponse("OK service mode active\r\n");
        return;

    case InterfaceCommandAction::SendToDirector:
        if (command.validateParameter && !config_.isValid(command.parameter, command.message.value))
        {
            char response[64]{};
            std::snprintf(response, sizeof(response), "ERR invalid %s\r\n",
                          parameterName(command.parameter));
            sendResponse(response);
            reportSettingFailure();
            return;
        }
        if (!sendToDirector(command.message))
        {
            sendResponse("ERR director busy\r\n");
            reportSettingFailure();
        }
        return;

    case InterfaceCommandAction::GetParameter:
        sendParameterResponse(command.parameter);
        reportSettingSucceeded();
        return;

    case InterfaceCommandAction::SaveConfig:
        config_.save();
        sendResponse("OK\r\n");
        reportSettingSucceeded();
        return;

    case InterfaceCommandAction::LoadConfig:
    {
        config_.load();
        const RuntimeConfig &active = config_.active();
        SystemMessage message{SystemCommand::LoadConfig, MessageSource::Interface,
                              active.targetRpm};
        message.aux = active.gains.kp;
        message.extra = active.gains.ki;
        message.detail = active.gains.kd;
        message.detail2 = static_cast<int32_t>(active.sampleTimeMs);
        if (!sendToDirector(message))
        {
            sendResponse("ERR director busy\r\n");
            reportSettingFailure();
        }
        return;
    }

    case InterfaceCommandAction::Error:
        sendResponse(command.response);
        if (command.reportFailure)
        {
            reportSettingFailure();
        }
        return;
    }
}

void InterfaceManager::handleInterfaceEvent(const InterfaceEvent &event)
{
    switch (event.type)
    {
    case InterfaceEventType::CommandOk:
        applyConfirmedCommand(event);
        sendResponse("OK\r\n");
        break;

    case InterfaceEventType::ManagerBusy:
        sendResponse("ERR manager busy\r\n");
        break;

    case InterfaceEventType::Unsupported:
        sendResponse("ERR unsupported command\r\n");
        break;

    case InterfaceEventType::Status:
        sendStatusResponse(event.status);
        break;
    }
}

void InterfaceManager::applyConfirmedCommand(const InterfaceEvent &event)
{
    switch (event.command)
    {
    case SystemCommand::SetTargetRpm:
        (void)config_.set(ConfigParameter::TargetRpm, event.value);
        break;

    case SystemCommand::SetKp:
        (void)config_.set(ConfigParameter::Kp, event.value);
        break;

    case SystemCommand::SetKi:
        (void)config_.set(ConfigParameter::Ki, event.value);
        break;

    case SystemCommand::SetKd:
        (void)config_.set(ConfigParameter::Kd, event.value);
        break;

    case SystemCommand::SetSampleTime:
        (void)config_.set(ConfigParameter::SampleTime, event.value);
        break;

    case SystemCommand::StopMotor:
        (void)config_.set(ConfigParameter::TargetRpm, 0);
        break;

    case SystemCommand::LoadConfig:
        (void)config_.set(ConfigParameter::TargetRpm, event.value);
        (void)config_.set(ConfigParameter::Kp, event.aux);
        (void)config_.set(ConfigParameter::Ki, event.extra);
        (void)config_.set(ConfigParameter::Kd, event.detail);
        (void)config_.set(ConfigParameter::SampleTime, event.detail2);
        break;

    default:
        break;
    }
}

void InterfaceManager::sendParameterResponse(ConfigParameter parameter)
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

    sendResponse(response);
}

void InterfaceManager::sendStatusResponse(const SystemStatus &status)
{
    char kp[16]{};
    char ki[16]{};
    char kd[16]{};
    formatGain(kp, sizeof(kp), status.kp);
    formatGain(ki, sizeof(ki), status.ki);
    formatGain(kd, sizeof(kd), status.kd);

    char response[288]{};
    std::snprintf(response, sizeof(response),
                  "STATUS current_rpm=%ld target_rpm=%ld pwm_duty=%ld direction=%s "
                  "kp=%s ki=%s kd=%s pid_output=%ld encoder_count=%ld controller=%s "
                  "feedback=%s control_state=%s\r\n",
                  static_cast<long>(status.currentRpm), static_cast<long>(status.targetRpm),
                  static_cast<long>(status.pwmDutyPermille), directionText(status.direction), kp,
                  ki, kd, static_cast<long>(status.pidOutput),
                  static_cast<long>(status.encoderCount),
                  status.controllerEnabled ? "ENABLED" : "STOPPED",
                  status.encoderFeedbackHealthy ? "OK" : "STALE",
                  controlStateText(status.controlState));
    sendResponse(response);
}

bool InterfaceManager::sendToDirector(const SystemMessage &message)
{
    return director_ != nullptr &&
           director_->sendEvent(message, config::kManagerQueueSendTimeoutMs);
}

void InterfaceManager::reportSettingSucceeded()
{
    (void)sendToDirector(
        SystemMessage{SystemCommand::SettingSucceeded, MessageSource::Interface, 0});
}

void InterfaceManager::reportSettingFailure()
{
    (void)sendToDirector(SystemMessage{SystemCommand::SettingFailed, MessageSource::Interface, 0});
}

} // namespace app

#include "app/ceo.hpp"

#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/ui_manager.hpp"
#include "app/interface_manager.hpp"
#include "app/motor_controller.hpp"

namespace app
{

CEO::CEO(InterfaceManager &interface, MotorController &motor)
    : interface_(interface), motor_(motor)
{
}

void CEO::initialize(UIManager &blink)
{
    blink_ = &blink;
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
    case SystemCommand::SetMotorTarget:
        if (!isMotorTargetValid(message.value))
        {
            interface_.sendResponse("ERR target range 0..180\r\n");
            reportSettingResult(false);
            return;
        }

        if (!sendMotorCommand(SystemCommand::SetMotorTarget, message.value))
        {
            interface_.sendResponse("ERR motor busy\r\n");
            reportSettingResult(false);
            return;
        }

        state_.motorTarget = message.value;
        state_.motorEnabled = true;
        interface_.sendResponse("OK target accepted\r\n");
        reportSettingResult(true);
        break;

    case SystemCommand::StopMotor:
        if (!sendMotorCommand(SystemCommand::StopMotor, 0))
        {
            interface_.sendResponse("ERR motor busy\r\n");
            reportSettingResult(false);
            return;
        }

        state_.motorTarget = 0;
        state_.motorEnabled = false;
        interface_.sendResponse("OK motor stopped\r\n");
        reportSettingResult(true);
        break;

    case SystemCommand::EncoderFeedback:
        state_.encoderDelta = message.value - state_.encoderPosition;
        state_.encoderPosition = message.value;
        break;

    case SystemCommand::StatusRequest:
    {
        char response[96]{};
        std::snprintf(response,
                      sizeof(response),
                      "STATUS target=%ld enabled=%u encoder=%ld delta=%ld\r\n",
                      static_cast<long>(state_.motorTarget),
                      state_.motorEnabled ? 1U : 0U,
                      static_cast<long>(state_.encoderPosition),
                      static_cast<long>(state_.encoderDelta));
        interface_.sendResponse(response);
        break;
    }

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

bool CEO::isMotorTargetValid(int32_t value) const
{
    return value >= config::kMotorTargetMin && value <= config::kMotorTargetMax;
}

bool CEO::sendMotorCommand(SystemCommand cmd, int32_t target)
{
    const MotorCommand motorCommand{cmd, target};
    return motor_.sendCommand(motorCommand, config::kManagerQueueSendTimeoutMs);
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

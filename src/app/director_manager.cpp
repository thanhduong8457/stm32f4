#include "app/director_manager.hpp"

#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/interface_manager.hpp"
#include "app/motor_controller.hpp"

namespace app
{

DirectorManager::DirectorManager(InterfaceManager &interface, MotorController &motor)
    : interface_(interface), motor_(motor)
{
}

void DirectorManager::initialize()
{
    queue_.create(config::kDirectorQueueLength);
}

bool DirectorManager::sendEvent(const SystemMessage &message, uint32_t timeoutMs)
{
    return queue_.send(message, timeoutMs);
}

void DirectorManager::processCommand(const SystemMessage &message)
{
    switch (message.cmd)
    {
    case SystemCommand::SetMotorTarget:
        if (!isMotorTargetValid(message.value))
        {
            interface_.sendResponse("ERR target range 0..180\r\n");
            return;
        }

        state_.motorTarget = message.value;
        state_.motorEnabled = true;
        sendMotorCommand(SystemCommand::SetMotorTarget, state_.motorTarget);
        interface_.sendResponse("OK target accepted\r\n");
        break;

    case SystemCommand::StopMotor:
        state_.motorTarget = 0;
        state_.motorEnabled = false;
        sendMotorCommand(SystemCommand::StopMotor, 0);
        interface_.sendResponse("OK motor stopped\r\n");
        break;

    case SystemCommand::EncoderFeedback:
        state_.encoderDelta = message.value - state_.encoderPosition;
        state_.encoderPosition = message.value;
        break;

    case SystemCommand::StatusRequest:
        printf("[Director] target=%ld enabled=%u encoder=%ld delta=%ld\r\n",
               static_cast<long>(state_.motorTarget),
               state_.motorEnabled ? 1U : 0U,
               static_cast<long>(state_.encoderPosition),
               static_cast<long>(state_.encoderDelta));
        break;

    case SystemCommand::InvalidCommand:
    default:
        interface_.sendResponse("ERR unsupported command\r\n");
        break;
    }
}

void DirectorManager::run()
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

const DirectorManager::State &DirectorManager::state() const
{
    return state_;
}

bool DirectorManager::isMotorTargetValid(int32_t value) const
{
    return value >= config::kMotorTargetMin && value <= config::kMotorTargetMax;
}

void DirectorManager::sendMotorCommand(SystemCommand cmd, int32_t target)
{
    const MotorCommand motorCommand{cmd, target};
    if (!motor_.sendCommand(motorCommand, config::kManagerQueueSendTimeoutMs))
    {
        printf("[Director] Motor queue full\r\n");
    }
}

} // namespace app

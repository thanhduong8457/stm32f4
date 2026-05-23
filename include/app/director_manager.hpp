#pragma once

#include <cstdint>

#include "app/system_messages.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

class InterfaceManager;
class MotorController;

class DirectorManager
{
public:
    struct State
    {
        int32_t motorTarget = 0;
        int32_t encoderPosition = 0;
        int32_t encoderDelta = 0;
        bool motorEnabled = false;
    };

    DirectorManager(InterfaceManager &interface, MotorController &motor);

    void initialize();
    void run();
    bool sendEvent(const SystemMessage &message, uint32_t timeoutMs = 0);
    void processCommand(const SystemMessage &message);
    const State &state() const;

private:
    bool isMotorTargetValid(int32_t value) const;
    void sendMotorCommand(SystemCommand cmd, int32_t target);

    InterfaceManager &interface_;
    MotorController &motor_;
    middleware::RtosQueue<SystemMessage> queue_;
    State state_{};
};

} // namespace app

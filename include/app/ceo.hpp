#pragma once

#include <cstdint>

#include "app/system_messages.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

class UIManager;
class InterfaceManager;
class MotorController;

class CEO
{
public:
    struct State
    {
        int32_t motorTarget = 0;
        int32_t encoderPosition = 0;
        int32_t encoderDelta = 0;
        bool motorEnabled = false;
    };

    CEO(InterfaceManager &interface, MotorController &motor);

    void initialize(UIManager &blink);
    void run();
    bool sendEvent(const SystemMessage &message, uint32_t timeoutMs = 0);
    void processCommand(const SystemMessage &message);
    const State &state() const;

private:
    bool isMotorTargetValid(int32_t value) const;
    bool sendMotorCommand(SystemCommand cmd, int32_t target);
    void reportSettingResult(bool succeeded);

    InterfaceManager &interface_;
    MotorController &motor_;
    UIManager *blink_ = nullptr;
    middleware::RtosQueue<SystemMessage> queue_;
    State state_{};
};

} // namespace app

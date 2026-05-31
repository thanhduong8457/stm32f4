#pragma once

#include <cstdint>

#include "app/system_messages.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

class EncoderManager;
class InterfaceManager;
class MotorController;
class PidManager;
class UIManager;

class CEO
{
public:
    struct State
    {
        int32_t currentRpm = 0;
        int32_t targetRpm = 0;
        int32_t pwmDutyPermille = 0;
        int32_t pidOutput = 0;
        int32_t encoderCount = 0;
        int32_t encoderDelta = 0;
        RotationDirection direction = RotationDirection::Stopped;
        bool controllerEnabled = false;
    };

    CEO(InterfaceManager &interface, MotorController &motor, PidManager &pid);

    void initialize(UIManager &ui, EncoderManager &encoder);
    void run();
    bool sendEvent(const SystemMessage &message, uint32_t timeoutMs = 0);
    void processCommand(const SystemMessage &message);
    const State &state() const;

private:
    bool routeToPid(PidCommandType type, int32_t value);
    bool routeToMotor(MotorCommandType type, int32_t value);
    bool routeToEncoder(EncoderCommandType type, int32_t value);
    void sendInterfaceEvent(const InterfaceEvent &event);
    void reportCommandResult(bool succeeded, const SystemMessage &request);
    void reportSettingResult(bool succeeded);
    void sendStatus();
    void syncState();

    InterfaceManager &interface_;
    MotorController &motor_;
    PidManager &pid_;
    EncoderManager *encoder_ = nullptr;
    UIManager *ui_ = nullptr;
    middleware::RtosQueue<SystemMessage> queue_;
    State state_{};
};

} // namespace app

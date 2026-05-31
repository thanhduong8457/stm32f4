#pragma once

#include <cstdint>

#include "app/configuration_manager.hpp"
#include "app/system_messages.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

class UIManager;
class EncoderManager;
class InterfaceManager;
class MotorController;

class CEO
{
public:
    struct State
    {
        int32_t targetRpm = 0;
        int32_t currentRpm = 0;
        int32_t pwmDutyPermille = 0;
        int32_t pidOutput = 0;
        int32_t encoderCount = 0;
        int32_t encoderDelta = 0;
        RotationDirection direction = RotationDirection::Stopped;
        bool controllerEnabled = false;
    };

    CEO(InterfaceManager &interface, MotorController &motor);

    void initialize(UIManager &blink, EncoderManager &encoder);
    void run();
    bool sendEvent(const SystemMessage &message, uint32_t timeoutMs = 0);
    void processCommand(const SystemMessage &message);
    const State &state() const;

private:
    bool applyConfigParameter(ConfigParameter parameter, int32_t value,
                              MotorCommandType motorCommand);
    bool applyActiveConfig();
    bool sendMotorCommand(MotorCommandType type, int32_t value);
    bool sendEncoderCommand(EncoderCommandType type, int32_t value);
    void sendGetResponse(ConfigParameter parameter);
    void sendStatusResponse();
    void syncMotorStatus();
    void reportSettingResult(bool succeeded);

    InterfaceManager &interface_;
    MotorController &motor_;
    EncoderManager *encoder_ = nullptr;
    UIManager *blink_ = nullptr;
    middleware::RtosQueue<SystemMessage> queue_;
    ConfigurationManager config_{};
    State state_{};
};

} // namespace app

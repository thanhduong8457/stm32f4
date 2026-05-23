#pragma once

#include "app/blink_task.hpp"
#include "app/director_manager.hpp"
#include "app/encoder_manager.hpp"
#include "app/interface_manager.hpp"
#include "app/motor_controller.hpp"

namespace app
{

class Application
{
public:
    Application(DirectorManager &director,
                InterfaceManager &interface,
                MotorController &motor,
                EncoderManager &encoder,
                BlinkTask &blink);

    void initialize();
    bool createTasks();
    [[noreturn]] void startScheduler();

    DirectorManager &director();
    InterfaceManager &interface();
    MotorController &motor();
    EncoderManager &encoder();
    BlinkTask &blink();

private:
    DirectorManager &director_;
    InterfaceManager &interface_;
    MotorController &motor_;
    EncoderManager &encoder_;
    BlinkTask &blink_;
};

} // namespace app

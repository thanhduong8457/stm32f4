#pragma once

#include "app/ui_manager.hpp"
#include "app/ceo.hpp"
#include "app/encoder_manager.hpp"
#include "app/interface_manager.hpp"
#include "app/motor_controller.hpp"

namespace app
{

class Application
{
public:
    Application(CEO &director,
                InterfaceManager &interface,
                MotorController &motor,
                EncoderManager &encoder,
                UIManager &blink);

    void initialize();
    bool createTasks();
    [[noreturn]] void startScheduler();

    CEO &director();
    InterfaceManager &interface();
    MotorController &motor();
    EncoderManager &encoder();
    UIManager &blink();

private:
    CEO &director_;
    InterfaceManager &interface_;
    MotorController &motor_;
    EncoderManager &encoder_;
    UIManager &blink_;
};

} // namespace app

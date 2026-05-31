#pragma once

#include "app/application.hpp"
#include "app/ceo.hpp"
#include "app/encoder_manager.hpp"
#include "app/interface_manager.hpp"
#include "app/motor_controller.hpp"
#include "app/ui_manager.hpp"
#include "platform/common/board.hpp"
#include "platform/stm32f1/drivers.hpp"

namespace platform::stm32f1
{

class Board final : public platform::IBoard
{
public:
    Board();

    void initializeClocks() override;
    app::Application &application() override;
    Uart1 &uart();

private:
    Uart1 uart1_{};
    Tim4Channel4Pwm pwm_{};
    Tim3Encoder encoder_{};
    Pc13Led led_{};
    app::InterfaceManager interface_{uart1_};
    app::MotorController motor_{pwm_};
    app::CEO director_{interface_, motor_};
    app::EncoderManager encoderManager_{encoder_};
    app::UIManager blink_{led_};
    app::Application application_{director_, interface_, motor_, encoderManager_, blink_};
};

Board &board();

} // namespace platform::stm32f1

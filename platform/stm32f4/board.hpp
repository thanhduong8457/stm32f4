#pragma once

#include "app/application.hpp"
#include "app/ceo.hpp"
#include "app/encoder_manager.hpp"
#include "app/interface_manager.hpp"
#include "app/motor_controller.hpp"
#include "app/pid_manager.hpp"
#include "app/ui_manager.hpp"
#include "platform/common/board.hpp"
#include "platform/stm32f4/drivers.hpp"

#if ADAS_USB_COMPOSITE
#include "platform/stm32f4/usb/usb_composite_device.hpp"
#endif

namespace platform::stm32f4
{

class Board final : public platform::IBoard
{
public:
    Board();

    void initializeClocks() override;
    app::Application &application() override;
    Uart1 &uart();
#if ADAS_USB_COMPOSITE
    UsbCompositeDevice &usb();
#endif

private:
    Uart1 uart1_{};
#if ADAS_USB_COMPOSITE
    UsbCompositeDevice usb_{};
#endif
    Tim4Channel4Pwm pwm_{};
    Tim3Encoder encoder_{};
    Pc13Led led_{};
#if ADAS_USB_COMPOSITE
    app::InterfaceManager interface_{usb_};
#else
    app::InterfaceManager interface_{uart1_};
#endif
    app::MotorController motor_{pwm_};
    app::PidManager pid_{};
    app::CEO director_{interface_, motor_, pid_};
    app::EncoderManager encoderManager_{encoder_};
    app::UIManager blink_{led_};
    app::Application application_{director_, interface_, motor_, pid_, encoderManager_, blink_};
};

Board &board();

} // namespace platform::stm32f4

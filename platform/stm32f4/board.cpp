#include "board.hpp"

#include "stm32f4xx.h"

namespace platform::stm32f4
{

Board::Board()
{
    uart1_.setRxSink(&interface_);
#if ADAS_USB_COMPOSITE
    usb_.setRxSink(&interface_);
#endif
}

void Board::initializeClocks()
{
    SystemInit();
    SystemCoreClockUpdate();
}

app::Application &Board::application()
{
    return application_;
}

Uart1 &Board::uart()
{
    return uart1_;
}

#if ADAS_USB_COMPOSITE
UsbCompositeDevice &Board::usb()
{
    return usb_;
}
#endif

Board &board()
{
    static Board instance;
    return instance;
}

} // namespace platform::stm32f4

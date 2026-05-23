#include "board.hpp"

#include "stm32f10x.h"

namespace platform::stm32f1
{

Board::Board()
{
    uart1_.setRxSink(&interface_);
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

Board &board()
{
    static Board instance;
    return instance;
}

} // namespace platform::stm32f1

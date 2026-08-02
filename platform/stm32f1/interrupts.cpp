#include "platform/stm32f1/board.hpp"

extern "C"
{

void USART1_IRQHandler(void)
{
    platform::stm32f1::board().uart().handleIrq();
}

int _write(int file, char *ptr, int len)
{
    (void)file;
    auto &uart = platform::stm32f1::board().uart();
    for (int i = 0; i < len; ++i)
    {
        uart.send(ptr[i]);
    }
    return len;
}

} // extern "C"

#include "platform/stm32f4/board.hpp"

extern "C"
{

void USART1_IRQHandler(void)
{
    platform::stm32f4::board().uart().handleIrq();
}

int _write(int file, char *ptr, int len)
{
    (void)file;
#if ADAS_USB_COMPOSITE
    auto &stream = platform::stm32f4::board().usb();
#else
    auto &uart = platform::stm32f4::board().uart();
#endif
    for (int i = 0; i < len; ++i)
    {
#if ADAS_USB_COMPOSITE
        stream.send(ptr[i]);
#else
        uart.send(ptr[i]);
#endif
    }
    return len;
}

} // extern "C"

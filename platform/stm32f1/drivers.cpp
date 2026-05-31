#include "drivers.hpp"

#include <cstdint>

#include "FreeRTOS.h"
#include "portmacro.h"
#include "stm32f10x.h"

namespace platform::stm32f1
{

void Uart1::setRxSink(hal::IUartRxSink *sink)
{
    rxSink_ = sink;
}

void Uart1::initialize()
{
    GPIO_InitTypeDef gpioInit{};
    USART_InitTypeDef usartInit{};
    NVIC_InitTypeDef nvicInit{};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpioInit.GPIO_Pin = GPIO_Pin_9;
    gpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
    gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpioInit);

    gpioInit.GPIO_Pin = GPIO_Pin_10;
    gpioInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpioInit);

    usartInit.USART_BaudRate = 115200;
    usartInit.USART_WordLength = USART_WordLength_8b;
    usartInit.USART_StopBits = USART_StopBits_1;
    usartInit.USART_Parity = USART_Parity_No;
    usartInit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usartInit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usartInit);

    USART_Cmd(USART1, ENABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    nvicInit.NVIC_IRQChannel = USART1_IRQn;
    nvicInit.NVIC_IRQChannelPreemptionPriority = 5;
    nvicInit.NVIC_IRQChannelSubPriority = 0;
    nvicInit.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicInit);
}

void Uart1::send(char ch)
{
    while ((USART1->SR & USART_SR_TXE) == 0)
    {
    }
    USART1->DR = static_cast<uint16_t>(ch & 0xFF);
}

void Uart1::send(const char *text)
{
    while (*text != '\0')
    {
        send(*text++);
    }
}

void Uart1::handleIrq()
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == RESET)
    {
        return;
    }

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    const uint8_t byte = static_cast<uint8_t>(USART_ReceiveData(USART1) & 0xFFU);
    if (rxSink_ != nullptr)
    {
        (void)rxSink_->onRxByteFromIsr(byte, &higherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

void Tim4Channel4Pwm::initialize()
{
    TIM_TimeBaseInitTypeDef timBase{};
    TIM_OCInitTypeDef timOc{};
    GPIO_InitTypeDef gpioInit{};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpioInit.GPIO_Pin = GPIO_Pin_9;
    gpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
    gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpioInit);

    timBase.TIM_Period = 1000 - 1;
    timBase.TIM_Prescaler = 72 - 1;
    timBase.TIM_ClockDivision = 0;
    timBase.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &timBase);

    timOc.TIM_OCMode = TIM_OCMode_PWM1;
    timOc.TIM_OutputState = TIM_OutputState_Enable;
    timOc.TIM_Pulse = 0;
    timOc.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC4Init(TIM4, &timOc);
    TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);

    TIM_Cmd(TIM4, ENABLE);
}

void Tim4Channel4Pwm::setDutyCyclePermille(uint16_t dutyPermille)
{
    if (dutyPermille > 1000U)
    {
        dutyPermille = 1000U;
    }

    TIM_SetCompare4(TIM4, dutyPermille);
}

void Tim3Encoder::initialize()
{
    GPIO_InitTypeDef gpioInit{};
    TIM_TimeBaseInitTypeDef timBase{};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpioInit.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpioInit.GPIO_Mode = GPIO_Mode_IPU;
    gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpioInit);

    timBase.TIM_Period = 0xFFFF;
    timBase.TIM_Prescaler = 0;
    timBase.TIM_ClockDivision = TIM_CKD_DIV1;
    timBase.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &timBase);

    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising,
                               TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
}

int32_t Tim3Encoder::read() const
{
    return static_cast<uint16_t>(TIM_GetCounter(TIM3));
}

void Pc13Led::initialize()
{
    GPIO_InitTypeDef gpioInit{};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    gpioInit.GPIO_Pin = GPIO_Pin_13;
    gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
    gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &gpioInit);
}

void Pc13Led::set(bool active)
{
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, active ? Bit_SET : Bit_RESET);
}

bool Pc13Led::get() const
{
    return GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) != 0;
}

} // namespace platform::stm32f1

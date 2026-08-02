#include "stm32f4xx.h"

#include "pwm.h"

void TIM4_PWM_Init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable clock for GPIO Port B, Enable clock for Timer 4
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // Configure PB9 as alternate function push-pull output
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure Timer 4 to generate PWM signals
    TIM_TimeBaseStructure.TIM_Period = 20000 - 1; // PWM period (50Hz)
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; // Prescaler (72 MHz / 72 = 1 MHz)
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    // Configure PWM output on Channel 4
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0; // 0% duty cycle
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC4Init(TIM4, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);

    // Enable Timer 4
    TIM_Cmd(TIM4, ENABLE);
}

// Set servo angle (0 to 180 degrees)
void set_servo_angle(unsigned int angle) {

    if (181 < angle) {
        angle = 180;
    }

    // Calculate pulse width (500 to 2500 microseconds) from angle
    uint16_t pulse_width = 500 + (angle * (2000.0/180.0));

    // Update PWM pulse width
    TIM_SetCompare4(TIM4, pulse_width);
}

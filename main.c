#include "stm32f4xx.h"                  // Device header

void delay_ms(uint32_t u32DelayInMs);

void delay_ms(uint32_t u32DelayInMs)
{
	
	while (u32DelayInMs) {
		TIM_SetCounter(TIM6, 0);
		while (TIM_GetCounter(TIM6) < 1000) {
		}
		--u32DelayInMs;
	}
}

int main(void)
{	
	GPIO_InitTypeDef gpioInit;
	TIM_TimeBaseInitTypeDef timInit;
	TIM_OCInitTypeDef pwmInit;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	gpioInit.GPIO_Mode = GPIO_Mode_OUT;
	gpioInit.GPIO_OType = GPIO_OType_PP;
	gpioInit.GPIO_Pin = GPIO_Pin_12;
	gpioInit.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gpioInit.GPIO_Speed = GPIO_High_Speed;
	
	GPIO_Init(GPIOD, &gpioInit);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	timInit.TIM_Prescaler = 84 - 1;
	timInit.TIM_Period = 0xFFFF;
	TIM_TimeBaseInit(TIM6, &timInit);
	TIM_Cmd(TIM6, ENABLE);
	
	/* timer 10 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM10, ENABLE);
	timInit.TIM_Prescaler = 168 - 1;
	timInit.TIM_Period = 10 - 1;
	TIM_TimeBaseInit(TIM10, &timInit);
	
	pwmInit.TIM_OCMode = TIM_OCMode_PWM1;
	pwmInit.TIM_OCPolarity = TIM_OCPolarity_High;
	pwmInit.TIM_Pulse = 2;
	pwmInit.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC1Init(TIM10, &pwmInit);
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	gpioInit.GPIO_Mode = GPIO_Mode_AF;
	gpioInit.GPIO_OType = GPIO_OType_PP;
	gpioInit.GPIO_Pin = GPIO_Pin_8;
	gpioInit.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gpioInit.GPIO_Speed = GPIO_High_Speed;
	
	GPIO_Init(GPIOB, &gpioInit);
	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_TIM10);
	
	TIM_Cmd(TIM10, ENABLE);
	
	while (1) {
		GPIO_SetBits(GPIOD, GPIO_Pin_12);
		delay_ms(100);
		TIM_SetCompare1(TIM10, 4);
		GPIO_ResetBits(GPIOD, GPIO_Pin_12);
		delay_ms(200);
		TIM_SetCompare1(TIM10, 8);
	}
}

#include "interrupt.h"

void init_interrupt(void)
{	
	// Enable clock for TIM2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	// TIM2 initialization for overflow every 500ms
	// Update Event (Hz) = SystemCoreClock / ((TIM_Prescaler + 1) * (TIM_Period + 1)*( RCR+1))
	// Update Event (Hz) = 72MHz / ((3599 + 1) * (9999 + 1)*(0+1)) = 2Hz (0.5s)
	//Thiet lap he so chia cho bo TIMER
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	// Thiet lap cho TIMER dem len
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	//Thiet lap gia tri cua chu ky
	TIM_TimeBaseInitStruct.TIM_Period = 9999;
	//thiet lap gia tri cho bo chia
	TIM_TimeBaseInitStruct.TIM_Prescaler = 3599;
	//Thiet lap gia tri RCR
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	// Thiet lap che do TimeBase
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);
	
	// Enable TIM2 interrupt
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	// Start TIM2
	TIM_Cmd(TIM2, ENABLE);

	NVIC_InitTypeDef  NVIC_InitStructure;
	
  	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
}

void TIM2_IRQHandler(void)
{
	// Checks whether the TIM2 interrupt has occurred or not
	if (TIM_GetITStatus(TIM2, TIM_IT_Update))
	{
		/* 
		insert the code here
		*/
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

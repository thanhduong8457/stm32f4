#include <iostream>
#include <string>

#include "stm32f4xx.h"
#include "system_timetick.h"

/*##################################################################################*/

void init_gpio(void);
void TIM2_IRQHandler();
void TIMER2_Configuration(void);

int main(void)
{
	init_gpio();
	delay_init();
	TIMER2_Configuration();

	while(true)
  	{
		GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
    	delay_ms(500);
	}
}

void init_gpio(void)
{
  	GPIO_InitTypeDef 	GPIO_InitStructure; 
  	//Enable clock GPIOD
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
  	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

  	GPIO_Init(GPIOD, &GPIO_InitStructure);  
 }

 void TIMER2_Configuration(void)
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
		GPIO_ToggleBits(GPIOD,GPIO_Pin_12);
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
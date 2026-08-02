#include "gpio.h"

void init_gpio(void)
{
  	GPIO_InitTypeDef 	GPIO_InitStructure; 
  	//Enable clock GPIOD
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
  	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

  	GPIO_Init(GPIOD, &GPIO_InitStructure);  
 }
 
void step(GPIO_TypeDef *IO, uint16_t dirPin, uint16_t stepperPin, int dir, int steps)
{
	if(dir)	GPIO_SetBits(IO, dirPin);
	else	GPIO_ResetBits(IO, dirPin);

  	for (int i = 0; i< steps; i++)
  	{
    	GPIO_SetBits(IO, stepperPin);
    	delay_us(500);
    	GPIO_ResetBits(IO, stepperPin);
    	delay_us(500);
 	}
}

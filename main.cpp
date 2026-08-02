#include "stm32f4xx.h"
#include "system_timetick.h"

void init_main(void);
void step(GPIO_TypeDef *IO, uint32_t dirPin, uint32_t stepperPin, bool dir, int steps, int delayTime);

int main(void)
{
	delay_init();
	init_main();

	//PIN_D_12: DIR
	//PIN_D_13: ENABLE
	GPIO_ResetBits(GPIOD, GPIO_Pin_13); //enable
	GPIO_SetBits(GPIOD, GPIO_Pin_14);

	int steps = 6400;

	while(true)
	{
		step(GPIOD, GPIO_Pin_12, GPIO_Pin_15, true, steps, 20);
	}
}

void init_main(void)
{
  	GPIO_InitTypeDef 	GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
  	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	
  	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void step(GPIO_TypeDef *IO, uint32_t dirPin, uint32_t stepperPin, bool dir, int steps, int delayTime)
{
	if(dir)
	{
		GPIO_SetBits(IO, dirPin);
	}
	else
	{
		GPIO_ResetBits(IO, dirPin);
	}
  	for (int i = 0; i< steps; i++)
  	{
    	GPIO_SetBits(IO, stepperPin);
    	delay_us(delayTime);
    	GPIO_ResetBits(IO, stepperPin);
    	delay_us(delayTime);
 	}
}

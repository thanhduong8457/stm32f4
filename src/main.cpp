#include <iostream>
#include <string>

#include "stm32f4xx.h"
#include "system_timetick.h"
#include "stdio.h"

void init_gpio(void);

int main(void) {
    delay_init();
    init_gpio();

    while(true) {
        #ifndef REGISTER
        GPIO_ToggleBits(GPIOD, GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);
            delay_ms(10);
        #else
        GPIOD->ODR |= (1<<12);
        delay_ms(50);
        GPIOD->ODR &= ~(1<<12);
        delay_ms(50);
        #endif
    }
}

void init_gpio(void)
{
  #ifndef REGISTER
  GPIO_InitTypeDef     GPIO_InitStructure; 
  //Enable clock GPIOD
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

  GPIO_Init(GPIOD, &GPIO_InitStructure);  
  #else
  /*GPIOD clock*/
  RCC->AHB1ENR = (1<<3);
  /*GPIOD config*/
  GPIOD->MODER &= ~(3<<24);
  GPIOD->MODER |= (1<<24);
  GPIOD->OTYPER &= ~(1<<12);
  #endif   
}

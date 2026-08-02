#include <iostream>
#include <string>

#include "stm32f4xx.h"
#include "stdio.h"
#include "gpio.h"

void init_gpio(void) {
#ifndef REGISTER
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); // Enable clock GPIOD

    GPIO_InitStructure.GPIO_Pin = 
    GPIO_Pin_12 
    | GPIO_Pin_13 
    | GPIO_Pin_14 ;
    // | GPIO_Pin_15;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

    GPIO_Init(GPIOD, &GPIO_InitStructure);
#else
    /*GPIOD clock*/
    RCC->AHB1ENR = (1 << 3);
    /*GPIOD config*/
    GPIOD->MODER &= ~(3 << 24);
    GPIOD->MODER |= (1 << 24);
    GPIOD->OTYPER &= ~(1 << 12);
#endif
}

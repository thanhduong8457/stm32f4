#include "stm32f4xx.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"

uint16_t N, Nx, Ny, Nz;
uint16_t countX=0;
uint16_t countY=0;

uint16_t countZ=0;

uint16_t acc1, acc2, acc3;

void vTask1(void * pvParams);
void vTask2(void * pvParams);
void vTask3(void * pvParams);
void vTask4(void * pvParams);

void init_gpio(void);

/// @brief 
/// @param  
/// @return 
int main(void) {
    SystemInit();
    SystemCoreClockUpdate();
    init_gpio();

    N = 10;
    Nx = 8;
    Ny = 10;
    Nz = 8;
    acc1 = 0;
    acc2 = 0;
    acc3 = 0;

    // Create a task
    // Stack and TCB are placed in CCM of STM32F4
    // The CCM block is connected directly to the core, which leads to zero wait states
    xTaskCreate (vTask1, "T1", 150, NULL, 1, NULL);
    xTaskCreate (vTask2, "T2", 150, NULL, 1, NULL);
    xTaskCreate (vTask3, "T2", 150, NULL, 1, NULL);
    xTaskCreate (vTask4, "T2", 150, NULL, 1, NULL);

    vTaskStartScheduler();

    printf("System Started!\n");
    vTaskStartScheduler();  // should never return
    return -1;
}

/// @brief 
/// @param  
void init_gpio(void) {
#ifndef REGISTER
    GPIO_InitTypeDef GPIO_InitStructure; 
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

/// @brief 
/// @param pvParams 
void vTask1(void * pvParams) {
    while(true) {
        BitAction set_value = BitAction::Bit_SET;
        while(true) {
            acc1 = acc1 + Nx;
            if (acc1 > N) {
                set_value = BitAction::Bit_SET;
                acc1 = acc1 - N;
                countX++;
                if (countX == Nx) {
                    // acc1 = 0;
                    // Nx = 0;
                    countX = 0;
                }
            }
            else {
                set_value = BitAction::Bit_RESET;
            }
            GPIO_WriteBit(GPIOD, GPIO_Pin_12, set_value);
            vTaskDelay(5); // delay10us(5);
            GPIO_WriteBit(GPIOD, GPIO_Pin_12, BitAction::Bit_RESET);
            vTaskDelay(5); // delay10us(5);

        }
    }
}

/// @brief 
/// @param pvParams 
void vTask2(void * pvParams) {
    while(true) {
        BitAction set_value = BitAction::Bit_SET;
        while(true) {
            acc2 = acc2 + Ny;
            if (acc2 > N) {
                set_value = BitAction::Bit_SET;
                acc2 = acc2 - N;
                countX++;
                if (countY == Ny) {
                    // acc2 = 0;
                    // Nx = 0;
                    countY = 0;
                }
            }
            else {
                set_value = BitAction::Bit_RESET;
            }
            GPIO_WriteBit(GPIOD, GPIO_Pin_13, set_value);
            vTaskDelay(5); // delay10us(5);
            GPIO_WriteBit(GPIOD, GPIO_Pin_13, BitAction::Bit_RESET);
            vTaskDelay(5); // delay10us(5);

        }
    }
}

/// @brief 
/// @param pvParams 
void vTask3(void * pvParams) {
    while(true) {
        BitAction set_value = BitAction::Bit_SET;
        while(true) {
            acc3 = acc3 + Nz;
            if (acc3 > N) {
                set_value = BitAction::Bit_SET;
                acc3 = acc3 - N;
                countZ++;
                if (countZ == Nz) {
                    // acc3 = 0;
                    // Nx = 0;
                    countZ = 0;
                }
            }
            else {
                set_value = BitAction::Bit_RESET;
            }
            GPIO_WriteBit(GPIOD, GPIO_Pin_14, set_value);
            vTaskDelay(5); // delay10us(5);
            GPIO_WriteBit(GPIOD, GPIO_Pin_15, BitAction::Bit_RESET);
            vTaskDelay(5); // delay10us(5);

        }
    }
}

/// @brief 
/// @param pvParams 
void vTask4(void * pvParams) {
    while(true) {
#ifndef REGISTER
        GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
        vTaskDelay(400);
#else
        GPIOD->ODR |= (1<<12);
        delay_ms(50);
        GPIOD->ODR &= ~(1<<12);
        delay_ms(50);
#endif
    }
}

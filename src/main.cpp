#include "stm32f4xx.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "gpio.h"

void vTask1(void * pvParams);
void vTask2(void * pvParams);
void vTask3(void * pvParams);

uint16_t N, Nx, Ny, Nz;
uint16_t acc1, acc2, acc3;

/// @brief 
/// @param  
/// @return 
int main(void) {
    // HAL_Init();
    
    // DoneStatus = true;
    // Rx_indx, 
    // Rx_data[24], 
    // Rx_Buffer[24], 
    // Transfer_cplt; // variables for receive and transmit data

    N = 10;
    Nx = 3;
    Ny = 3;
    Nz = 3;
    acc1 = 0;
    acc2 = 0;
    acc3 = 0;

    SystemInit();
    SystemCoreClockUpdate();
    
    init_gpio();  // might be handle for direction
    // UART1_Init(); // MX_UART4_Init(); // handle data receive

    // Create a task
    // Stack and TCB are placed in CCM of STM32F4
    // The CCM block is connected directly to the core, which leads to zero wait states
    xTaskCreate (vTask1, "T1", 150, NULL, 1, NULL);
    // xTaskCreate (vTask2, "T2", 150, NULL, 1, NULL);
    // xTaskCreate (vTask3, "T2", 150, NULL, 1, NULL);

    vTaskStartScheduler();

    printf("System Started!\n");
    vTaskStartScheduler();  // should never return

    while (1) {
        // if (DoneStatus == true) {HAL_UART_Receive_IT(&huart4, (uint8_t*) Rx_Buffer, 24);}	//Kich hoat UART nhan
        // HAL_UART_Receive_IT(&huart4, (uint8_t *)Rx_Buffer, 24);
    }
}


/// @brief // caculate for arm 1
/// @param pvParams 
void vTask1(void * pvParams) {
    BitAction set_value = BitAction::Bit_SET;
    while(true) {
        acc1 = acc1 + Nx;
        if (acc1 > N) {
            set_value = BitAction::Bit_SET;
            acc1 = acc1 - N;
            // countX++;
            // if (countX == Nx) {
            //     acc1 = 0;
            //     Nx = 0;
            //     countX = 0;
            // }
        }
        else {
            set_value = BitAction::Bit_RESET;
        }
        GPIO_WriteBit(GPIOD, GPIO_Pin_12, set_value);
        vTaskDelay(5); // delay10us(5);
    }
}

/// @brief 
/// @param pvParams 
void vTask2(void * pvParams) {
    while(true) {
        // caculate for arm 2
        acc2 = acc2 + Ny;
        if (acc2 > N) {
            acc2 = acc2 - N;
            GPIO_SetBits(GPIOD, GPIO_Pin_13);
            vTaskDelay(5); // delay10us(5);
            GPIO_ResetBits(GPIOD, GPIO_Pin_13);
            vTaskDelay(5); // delay10us(5);
            
            // countY++;
            // if (countY == Ny) {
            //     acc2 = 0;
            //     Ny = 0;
            //     countY = 0;
            // }
        }
    }
}

/// @brief 
/// @param pvParams 
void vTask3(void * pvParams) {
    while(true) {

        // caculate for arm 3
        acc3 = acc3 + Nz;
        if (acc3 > N) {
            acc3 = acc3 - N;
            GPIO_SetBits(GPIOD, GPIO_Pin_14);
            vTaskDelay(5); // delay10us(5);
            GPIO_ResetBits(GPIOD, GPIO_Pin_14);
            vTaskDelay(5); // delay10us(5);

            // countZ++;
            // if (countZ == Nz) {
            //     acc3 = 0;
            //     Nz = 0;
            //     countZ = 0;
            // }
        }

        if ((Nx == 0) & (Ny == 0) & (Nz == 0)) {
            // DoneStatus = true;
            // HAL_TIM_Base_Stop_IT(&htim2); // stop trigger timer interrupt during empty buffer
        }

    }
}

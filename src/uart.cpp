#include "stm32f4xx.h"
#include "uart.h"
#include "stdio.h"
#include "stdlib.h"
// double DegreePerStep = 1.8 / 16;
// double PulseOfX = 0; 
// double PulseOfY = 0; 
// double PulseOfZ = 0;
// double PrePulseOfX = 0;
// double PrePulseOfY = 0;
// double PrePulseOfZ = 0;

// Initialize UART1
void UART1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure; // Define NVIC_InitStructure here

    // Enable UART1 and GPIO clock
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // Configure UART1 pins: Tx (PA9) and Rx (PA10)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Configure UART1
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    
    USART_Cmd(USART1, ENABLE); // Enable UART1
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // Enable USART1 RX interrupt

    // Configure and enable USART1 interrupt
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// UART1 interrupt handler
void USART1_IRQHandler(void) {
    // char received_char;
    // DoneStatus = false;
    // char X[7], Y[7], Z[7];
    // int32_t pulseX, pulseY, pulseZ;
    // double tempX = 0;
    // double tempY = 0;
    // double tempZ = 0;

    // if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
    //     received_char = USART_ReceiveData(USART1);
    //     UART1_SendChar(received_char); // Handle received character
    // }

    // // parse data from buffer
    // for (int i = 1; i < 8; i++) X[i - 1] = Rx_Buffer[i]; // x[0:7] store rx_buffer[1:7]
    // for (int i = 9; i < 16; i++) Y[i - 9] = Rx_Buffer[i]; // y[0:7] store rx_buffer[9:15]
    // for (int i = 17; i < 24; i++) Z[i - 17] = Rx_Buffer[i]; // z[0:7] store rx_buffer[17:23]

    // tempX = atof((const char*)&X);
    // tempY = atof((const char*)&Y);
    // tempZ = atof((const char*)&Z);

    // PulseOfX = tempX / (DegreePerStep);
    // PulseOfY = tempY / (DegreePerStep);
    // PulseOfZ = tempZ / (DegreePerStep);

    // // pulse need to each axis
    // pulseX = PulseOfX - PrePulseOfX;
    // pulseY = PulseOfY - PrePulseOfY;
    // pulseZ = PulseOfZ - PrePulseOfZ;

    // // Pulse to each axis
    // Nx = abs(pulseX);
    // Ny = abs(pulseY);
    // Nz = abs(pulseZ);

    // acc1 = Nx;
    // acc2 = Ny;
    // acc3 = Nz;

    // PrePulseOfX = PulseOfX;
    // PrePulseOfY = PulseOfY;
    // PrePulseOfZ = PulseOfZ;

    // HAL_TIM_Base_Start_IT(&htim2); // enable IT off timer 2
}

// Send a character via UART1
void UART1_SendChar(char ch) {
    // Wait until transmit data register is empty
    while (!(USART1->SR & USART_SR_TXE));
    // Send a character
    USART1->DR = (ch & 0xFF);
}

// Redirect fputc() to UART1
int fputc(int ch, FILE *f) {
    UART1_SendChar(ch);
    return ch;
}

// Implementing _write for printf
// This function is used by printf to send character output
int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        UART1_SendChar(*ptr++);
    }
    return len;
}

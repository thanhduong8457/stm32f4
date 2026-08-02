#include "stm32f4xx.h"

#include "uart.h"
#include "pwm.h" // for call set_angle function

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

    // Enable UART1
    USART_Cmd(USART1, ENABLE);

    // Enable USART1 RX interrupt
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    // Configure and enable USART1 interrupt
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// UART1 interrupt handler
void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        char received_char = USART_ReceiveData(USART1);
        // Handle received character
        UART1_SendChar(received_char);
    }

    // if (false == revert) {
    //     share_value++;
    // } else {
    //     share_value--;
    // }

    // if (share_value >= 180 || share_value <= 0) {
    //     if (false == revert) {
    //         revert = true;
    //     } else {
    //         revert = false;
    //     }
    // }

    switch (share_value) {
    case 0:
        share_value = 45;
        break;

    case 45:
        share_value = 90;
        break;

    case 90:
        share_value = 135;
        break;

    case 135:
        share_value = 180;
        break;

    default:
        share_value = 0;
        break;
    }

    set_servo_angle(share_value);
    printf("current angle is %d degree\r\n", share_value);

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
    int i;
    for (i = 0; i < len; i++) {
        UART1_SendChar(*ptr++);
    }
    return len;
}

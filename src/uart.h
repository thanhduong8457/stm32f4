#include <stdio.h>

extern volatile unsigned int share_value;
extern bool revert;

extern "C" { // use extern for link C funtions
    void USART1_IRQHandler(void);
    int _write(int file, char *ptr, int len);
}

// Function prototypes
void UART1_Init(void);
void UART1_SendChar(char ch);

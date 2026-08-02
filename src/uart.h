extern uint16_t N, Nx, Ny, Nz;
extern uint16_t acc1, acc2, acc3;

extern "C" { // use extern for link C funtions
    void USART1_IRQHandler(void);
    int _write(int file, char *ptr, int len);
}

// Function prototypes
void UART1_Init(void);
void UART1_SendChar(char ch);

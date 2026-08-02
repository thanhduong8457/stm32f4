#ifndef __uart_H
#define __uart_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdint.h"

#include "cJSON.h"

#define USE_UART4
#define RX_BUFFER_SIZE 64

typedef struct {
	int8_t *Buffer;
	uint16_t Size;
	uint16_t Num;
	uint16_t In;
	uint16_t Out;
} USART_t;

extern volatile int theta_1;
extern volatile int theta_2;
extern volatile int theta_3;

extern int last_theta_1;
extern int last_theta_2;
extern int last_theta_3;

extern int denta_1;
extern int denta_2;
extern int denta_3;

extern int statuss_1;
extern int statuss_2;
extern int statuss_3;

extern int dir_1;
extern int dir_2;
extern int dir_3;


#define USART2_BUFFER_SIZE 100

//USART2 default settings
#ifndef USART2_HARDWARE_FLOW_CONTROL
#define USART2_HARDWARE_FLOW_CONTROL		USART_HardwareFlowControl_None
#endif
#ifndef USART2_MODE
#define USART2_MODE						USART_Mode_Tx | USART_Mode_Rx
#endif
#ifndef USART2_PARITY
#define USART2_PARITY					USART_Parity_No
#endif
#ifndef USART2_STOP_BITS
#define USART2_STOP_BITS					USART_StopBits_1
#endif
#ifndef USART2_WORD_LENGTH
#define USART2_WORD_LENGTH				USART_WordLength_8b
#endif

extern void USART_Putc(USART_TypeDef* USARTx, volatile char c);
extern void USART_Puts(USART_TypeDef* USARTx, char* str);
extern uint8_t USART_Getc(USART_TypeDef* USARTx);
extern uint16_t USART_Gets(USART_TypeDef* USARTx, char* buffer, uint16_t bufsize);
extern uint8_t USART_FindCharacter(USART_TypeDef* USARTx, volatile char c);
extern uint8_t USART_BufferEmpty(USART_TypeDef* USARTx);
extern uint8_t USART_BufferFull(USART_TypeDef* USARTx);
extern void USART_ClearBuffer(USART_TypeDef* USARTx);

void USART_INT_InsertToBuffer(USART_t* u, uint8_t c);
USART_t* USART_INT_GetUsart(USART_TypeDef* USARTx);
void clear_rx_buffer(void);
void json_data_process(char *data);
void do_something(void);

#ifdef __cplusplus
}
#endif
#endif

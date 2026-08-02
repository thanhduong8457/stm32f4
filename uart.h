#ifndef __uart_H
#define __uart_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx.h"

void init_uart(void);
int stringLength(char *str);
void send_data(char data[]);

#ifdef __cplusplus
}
#endif
#endif

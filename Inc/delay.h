#ifndef __delay_H
#define __delay_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx.h"

void init_delay(void);
void delay_ms(uint32_t u32DelayInMs);
void delay_us(uint32_t u32DelayInUs);

#ifdef __cplusplus
}
#endif
#endif

#ifndef __interrupt_H
#define __interrupt_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx.h"

void init_interrupt(void);
void TIM2_IRQHandler(void);

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

#ifdef __cplusplus
}
#endif
#endif

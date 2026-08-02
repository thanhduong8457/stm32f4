#ifndef __gpio_H
#define __gpio_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx.h"
#include "delay.h"

void init_gpio(void);
void step(GPIO_TypeDef *IO, uint16_t dirPin, uint16_t stepperPin, int dir, int steps);

#ifdef __cplusplus
}
#endif
#endif

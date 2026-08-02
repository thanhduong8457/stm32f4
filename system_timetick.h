
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/*  These function configure scheduler timer
    Sampling time is set by changing value PERIOD
    in the file "system_timetick.c"
*/

extern uint32_t tick_count;
extern uint32_t tick_flag;


void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

u32 millis(void);
u32 micros(void);

void delay_init(void);
void delay_ms(u32 nms);
void delay_us(u32 nus);
void delay( uint32_t ms );
void delayMicroseconds(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */



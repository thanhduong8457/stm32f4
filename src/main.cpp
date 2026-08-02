#include "stm32f4xx.h"
#include "system_timetick.h"
#include "system_stm32f4xx.h"

#include "uart.h"
#include "gpio.h"
#include "pwm.h"

volatile unsigned int share_value = 0;
bool revert = false;

int main(void) {
    SystemInit();
    SystemCoreClockUpdate();

    delay_init(); // Initialize time tick
    gpio_init(); // Initialize GPIO for pin PC13
    TIM4_PWM_Init(); // Initialize PWM on TIM4
    UART1_Init(); // Initialize UART

    set_servo_angle(share_value);

    while (1) {
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        delay_ms(500);
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        delay_ms(500);
        // printf("current angle is %d degree\r\n", share_value);
    }
}

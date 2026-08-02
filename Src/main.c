#include "stm32f4xx.h"
#include "stdio.h"

#include "gpio.h"
#include "delay.h"
#include "interrupt.h"
#include "uart.h"

#include "FreeRTOS.h"
#include "task.h"

#define stepp 6400
#define step_per_degree 200/360
#define gear_ratio 4

static void task1_handler(void* parameters);
static void task2_handler(void* parameters);
static void task3_handler(void* parameters);
static void main_task(void* parameters);

volatile int theta_1;
volatile int theta_2;
volatile int theta_3;

int last_theta_1;
int last_theta_2;
int last_theta_3;

int denta_1;
int denta_2;
int denta_3;

int statuss_1;
int statuss_2;
int statuss_3;

int dir_1;
int dir_2;
int dir_3;

int current_theta_1 = 0;
int current_theta_2 = 0;
int current_theta_3 = 0;

TaskHandle_t task1_handle;
TaskHandle_t task2_handle;
TaskHandle_t task3_handle;
TaskHandle_t main_task_handle;

BaseType_t status;

int main(void)
{
	SystemInit();
	SystemCoreClockUpdate();
	init_gpio();
	init_delay();
	//init_interrupt();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	init_uart();	
	
	status = xTaskCreate(task1_handler, "Task-1", 200, NULL, 2, &task1_handle);	
	configASSERT(status == pdPASS);
	
	status = xTaskCreate(task2_handler, "Task-2", 200, NULL, 2, &task2_handle);
	configASSERT(status == pdPASS);
	
	status = xTaskCreate(task3_handler, "Task-3", 200, NULL, 2, &task3_handle);
	configASSERT(status == pdPASS);

	// status = xTaskCreate(main_task, "main_task", 200, NULL, 2, &main_task_handle);
	// configASSERT(status == pdPASS);
	
	vTaskStartScheduler();

	while(1)
	{
		// system is not going to run this line
	}
}

static void task1_handler(void* parameters)
{
	while(1)
	{
		if(current_theta_1>theta_1){
			GPIO_SetBits(GPIOD, GPIO_Pin_10);
			current_theta_1--;

			GPIO_SetBits(GPIOD, GPIO_Pin_11);
			vTaskDelay(pdMS_TO_TICKS(1));
    		GPIO_ResetBits(GPIOD, GPIO_Pin_11);
    		vTaskDelay(pdMS_TO_TICKS(1));
		}
		else if(current_theta_1<theta_1){
			GPIO_ResetBits(GPIOD, GPIO_Pin_10);
			current_theta_1++;

			GPIO_SetBits(GPIOD, GPIO_Pin_11);
			vTaskDelay(pdMS_TO_TICKS(1));
    		GPIO_ResetBits(GPIOD, GPIO_Pin_11);
    		vTaskDelay(pdMS_TO_TICKS(1));
		}

		taskYIELD();
	}
}

static void task2_handler(void* parameters){
	while(1){
		if(current_theta_2>theta_2){
			GPIO_SetBits(GPIOD, GPIO_Pin_12);
			current_theta_2--;

			GPIO_SetBits(GPIOD, GPIO_Pin_13);
			vTaskDelay(pdMS_TO_TICKS(1));
    		GPIO_ResetBits(GPIOD, GPIO_Pin_13);
    		vTaskDelay(pdMS_TO_TICKS(1));
		}
		else if(current_theta_2<theta_2){
			GPIO_ResetBits(GPIOD, GPIO_Pin_12);
			current_theta_2++;

			GPIO_SetBits(GPIOD, GPIO_Pin_13);
			vTaskDelay(pdMS_TO_TICKS(1));
    		GPIO_ResetBits(GPIOD, GPIO_Pin_13);
    		vTaskDelay(pdMS_TO_TICKS(1));
		}

		taskYIELD();
	}
}

static void task3_handler(void* parameters)
{
	while(1)
	{
		if(current_theta_3>theta_3){
			GPIO_SetBits(GPIOD, GPIO_Pin_14);
			current_theta_3--;

			GPIO_SetBits(GPIOD, GPIO_Pin_15);
			vTaskDelay(pdMS_TO_TICKS(1));
    		GPIO_ResetBits(GPIOD, GPIO_Pin_15);
    		vTaskDelay(pdMS_TO_TICKS(1));
		}
		else if(current_theta_3<theta_3){
			GPIO_ResetBits(GPIOD, GPIO_Pin_14);
			current_theta_3++;

			GPIO_SetBits(GPIOD, GPIO_Pin_15);
			vTaskDelay(pdMS_TO_TICKS(1));
    		GPIO_ResetBits(GPIOD, GPIO_Pin_15);
    		vTaskDelay(pdMS_TO_TICKS(1));
		}

		taskYIELD();
	}
}

void main_task(void *parameters)
{	
	int temp = 0;
	char JSON[64];
	char temp_str[64];

 	while(1)
	{
		for(int i=0; i<64; i++)
		{
			JSON[i] = 0;
			temp_str[i] = 0;
		}
		
		sprintf(temp_str, "%d", temp);
		
		//printf("Hello %d times, It work!\r\n", temp);
		strcat(JSON, "\"temp\":\"");
		strcat(JSON, temp_str);
		strcat(JSON, "\"\n");
		printf(JSON);
		temp++;
		if(temp>100) temp = 0;
		vTaskDelay(pdMS_TO_TICKS(1000));
		taskYIELD();
	}
}	

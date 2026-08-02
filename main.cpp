#include <iostream>
#include <string>

#include "stm32f4xx.h"
#include "system_timetick.h"
#include "PID.h"
#include "fuzzy_ctrl.h"
#include "stdio.h"

/*##################################################################################*/
int count = 0;
float ref = 1000;
double y = 0;
double u = 0;
double temp;
double error = 0, error_dot = 0, pre_error = 0;

static double uk1 = 0;
static double uk2 = 0;
static double yk1 = 0;
static double yk2 = 0;

double a = 8.594603E-05;
double b = 8.271147E-05;
double c = -1.88986;
double d = 0.8912866;

/*##################################################################################*/

void init_uart4(void);
void init_gpio(void);
void init_DMA(void);

double plant_motor(double uk);
void cout(std::string data);
float set_ref(float duration, int &temp);

int main(void)
{
	init_uart4();
  init_gpio();

  //setup PID controller
	PID *pid;
	pid = new PID(8, 3, 5, 0.02, "auto");

  FUZZY *fuzzy;
	fuzzy = new FUZZY();

  fuzzy->rule_fuzzy();

	while(true){		 
    count++;
    temp++;
    ref = set_ref(1000, count);
		
	  //fuzzy compute
	  error = ref - y;
	  error_dot = (error - pre_error)/0.01;
	  pre_error = error;
		
	  fuzzy->value_hlt_e(error);
	  fuzzy->value_hlt_e_dot(error_dot);
	  fuzzy->find_beta();	
			
	  pid->set_Kp(fuzzy->defuzzy_kp());
	  pid->set_Ki(fuzzy->defuzzy_ki());
	  pid->set_Kd(fuzzy->defuzzy_kd());

	  u = pid->compute(ref, y);	
	  y = plant_motor(u);

    if(temp>50)
    {
      temp = 0;
      GPIO_ToggleBits(GPIOD, GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);
    }

		cout("ref: " + std::to_string(ref) + 
      "  y: " + std::to_string(y) + 
      "  Mode: " + pid->get_mode() + 
      "  Kp: " + std::to_string(pid->get_Kp()) + 
      "  Ki: " + std::to_string(pid->get_Ki()) + 
      "  Kd: " + std::to_string(pid->get_Kd()) + 
      "  Ts: " + std::to_string(pid->get_Ts()) + 
      "\r\n");

    printf("thanh duong edit this file\n\r");

		delay_ms(20);
	}
}

void init_uart4(void)
{
  GPIO_InitTypeDef 	GPIO_InitStructure; 
	USART_InitTypeDef USART_InitStructure;   
   
  /* Enable GPIO clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  /* Enable UART clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

  /* Connect UART4 pins to AF2 */  
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_UART4);  //PA0
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_UART4);  //PA1

  /* GPIO Configuration for UART4 Tx */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* GPIO Configuration for USART Rx */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
       
  /* USARTx configured as follow:
		- BaudRate = 115200 baud  
    - Word Length = 8 Bits
    - One Stop Bit
    - No parity
    - Hardware flow control disabled (RTS and CTS signals)
    - Receive and transmit enabled
  */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(UART4, &USART_InitStructure);

  /* Enable USART */
  USART_Cmd(UART4, ENABLE);
}

void init_gpio(void)
{
  	GPIO_InitTypeDef 	GPIO_InitStructure; 

	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
  	/* GPIO Configuration for UART4 Tx */
  	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	
  	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  	GPIO_Init(GPIOD, &GPIO_InitStructure);     
}

void init_DMA(void)
{
  // DMA_TypeDef dmaInit;
  // RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
}

void cout(std::string data)
{
	for(int i=0; i<data.size()+1; i++)
	{
		USART_SendData(UART4,(uint8_t)data[i]);
    while (USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
	}
}

float set_ref(float duration, int &temp)
{
	int ref = 0;
	if(temp < (duration/2)) ref = 20;
	else if(temp < duration) ref = 5;
	else temp = 0;

	return ref;
}

double plant_motor(double uk)
{
	double yk = a*uk1 + b*uk2 - c*yk1 - d*yk2;
	uk1 = uk;
	uk2 = uk1;
	yk1 = yk;
	yk2 = yk1;
	
	return yk;
}
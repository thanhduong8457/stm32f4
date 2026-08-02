#include "stm32f4xx.h"
#include "system_timetick.h"

int stepp = 6400;

void init_gpio(void);
void TIM2_IRQHandler(void);
void TIMER2_Configuration(void);
void init_main(void);

int stringLength(char *str);
void send_data(char data[]);
void step(GPIO_TypeDef *IO, uint16_t dirPin, uint16_t stepperPin, int dir, int steps, u32 delayTime);

int main(void)
{
	init_gpio();
	delay_init();
	TIMER2_Configuration();
	init_main();
	
	//PIN_D_12: DIR
	//PIN_D_13: ENABLE
	GPIO_ResetBits(GPIOD, GPIO_Pin_13); //enable
	GPIO_SetBits(GPIOD, GPIO_Pin_14);

	while(1)
	{
		GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
		step(GPIOD, GPIO_Pin_12, GPIO_Pin_15, 1, stepp, 20);
    	delay_ms(500);
		send_data("HOLA:\r\n");
	}
}

void TIM2_IRQHandler(void)
{
	// Checks whether the TIM2 interrupt has occurred or not
	if (TIM_GetITStatus(TIM2, TIM_IT_Update))
	{
		//GPIO_ToggleBits(GPIOD,GPIO_Pin_12);
		send_data("HOLA:\r\n");
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

int stringLength(char *str)  
{  
    int i=0;  
    while(str[i] != '\0') i++;    
    return i;  
} 

void send_data(char data[])
{
	for(int i=0; i<stringLength(data)+1; i++)
	{
		while (USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
		USART_SendData(UART4,(uint8_t)data[i]);
	}
}


void step(GPIO_TypeDef *IO, uint16_t dirPin, uint16_t stepperPin, int dir, int steps, u32 delayTime)
{
	if(dir)
	{
		GPIO_SetBits(IO, dirPin);
	}
	else
	{
		GPIO_ResetBits(IO, dirPin);
	}
  	for (int i = 0; i< steps; i++)
  	{
    	GPIO_SetBits(IO, stepperPin);
    	delay_us(delayTime);
    	GPIO_ResetBits(IO, stepperPin);
    	delay_us(delayTime);
 	}
}

void init_main(void)
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
  	//Enable clock GPIOD
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
  	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

  	GPIO_Init(GPIOD, &GPIO_InitStructure);  
 }

 void TIMER2_Configuration(void)
{	
	// Enable clock for TIM2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	// TIM2 initialization for overflow every 500ms
	// Update Event (Hz) = SystemCoreClock / ((TIM_Prescaler + 1) * (TIM_Period + 1)*( RCR+1))
	// Update Event (Hz) = 72MHz / ((3599 + 1) * (9999 + 1)*(0+1)) = 2Hz (0.5s)
	//Thiet lap he so chia cho bo TIMER
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	// Thiet lap cho TIMER dem len
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	//Thiet lap gia tri cua chu ky
	TIM_TimeBaseInitStruct.TIM_Period = 9999;
	//thiet lap gia tri cho bo chia
	TIM_TimeBaseInitStruct.TIM_Prescaler = 3599;
	//Thiet lap gia tri RCR
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	// Thiet lap che do TimeBase
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);
	
	// Enable TIM2 interrupt
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	// Start TIM2
	TIM_Cmd(TIM2, ENABLE);

	NVIC_InitTypeDef  NVIC_InitStructure;	
	
  	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
}

#include "uart.h"
#include "misc.h"

char rx_buffer[RX_BUFFER_SIZE];
int rx_index = 0;

cJSON *my_data_json;

FILE __stdout;

int fputc(int ch, FILE *f){
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
  USART_SendData(USART2, (uint8_t)ch);
  return ch;
}

/* Set variables for buffers */
int8_t USART2_Buffer[USART2_BUFFER_SIZE];
USART_t F4_USART2 = {USART2_Buffer, USART2_BUFFER_SIZE, 0, 0, 0};

void init_uart(void){
  	GPIO_InitTypeDef 	GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
  	NVIC_InitTypeDef  NVIC_InitStructure;

  /* Enable GPIO clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  /* Enable UART clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

  /* Connect USART2 pins to AF2 */  
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);  //PA2
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);  //PA3

  /* GPIO Configuration for USART2 Tx */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* GPIO Configuration for USART Rx */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* USARTx configured as follow:
		- BaudRate = 115200 baud  
    - Word Length = 8 Bits
    - One Stop Bit
    - No parity
    - Hardware flow control disabled (RTS and CTS signals)
    - Receive and transmit enabled
  */

  /* Disable */
	USART_Cmd(USART2, DISABLE);
	/* Deinit first */
	USART_DeInit(USART2);

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART2, &USART_InitStructure);
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

  /* Enable USART */
  USART_Cmd(USART2, ENABLE);
}

void USART2_IRQHandler(void){
	//Check if interrupt was because data is received
	if (USART_GetITStatus(USART2, USART_IT_RXNE)) {
		//Put received data into internal buffer
		//USART_INT_InsertToBuffer(&F4_USART2, USART2->DR);
      	char ReceiveData = NULL;
      	ReceiveData = USART_ReceiveData(USART2);
      	if(ReceiveData != '\n'){
        	rx_buffer[rx_index++] = ReceiveData;
      	}
      	else{
        	// printf("Data RX_Buffer: %s\n", rx_buffer);

        	json_data_process(rx_buffer);

        	// do_something();

        	clear_rx_buffer();
      	}
	}
}

void do_something(void)
{
  	denta_1 = theta_1 - last_theta_1;
    denta_2 = theta_2 - last_theta_2;
    denta_3 = theta_3 - last_theta_3;
		
	if(denta_1<0){
		dir_1 = 0;
		denta_1 = -denta_1;
	}
	else dir_1 = 1;

	denta_2 = theta_2 - last_theta_2;

	if(denta_2<0){
		dir_2 = 0;
		denta_2 = -denta_2;
	}
	else dir_2 = 1;

	denta_3 = theta_3 - last_theta_3;

	if(denta_3<0){
		dir_3 = 0;
		denta_3 = -denta_3;
	}
	else dir_3 = 1;

	last_theta_1 = theta_1;
	last_theta_2 = theta_2;
	last_theta_3 = theta_3;

	statuss_1 = 1;
	statuss_2 = 1;
	statuss_3 = 1;
}

void json_data_process(char *data){
  my_data_json = cJSON_Parse(data);

  if(cJSON_GetObjectItem(my_data_json, "theta1")){
    theta_1 = atoi(cJSON_GetObjectItem(my_data_json, "theta1")->valuestring);
    printf("The value of theta_1 is: %d\n", theta_1);
  }

  if(cJSON_GetObjectItem(my_data_json, "theta2")){
    theta_2 = atoi(cJSON_GetObjectItem(my_data_json, "theta2")->valuestring);
    printf("The value of theta_2 is: %d\n", theta_2);
  }

  if(cJSON_GetObjectItem(my_data_json, "theta3")){
    theta_3 = atoi(cJSON_GetObjectItem(my_data_json, "theta3")->valuestring);
    printf("The value of theta_3 is: %d\n", theta_3);
  }

  cJSON_Delete(my_data_json);
}

void clear_rx_buffer(void){
  for(int j=0;j<RX_BUFFER_SIZE;j++) rx_buffer[j] = 0;
  rx_index = 0;
}

void USART2_PutChar(char data){
  //while(!USART_GetITStatus(USART2, USART_FLAG_TC));
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
  USART_SendData(USART2, (uint8_t)data);
  //USART2->DR = (uint16_t)(data & 0x01FF);
}

void USART2_PutString(char *data){
  while(*data)
  {
    USART2_PutChar(*data++);
  }
}

uint8_t USART_Getc(USART_TypeDef* USARTx) {
	int8_t c = 0;
	USART_t* u = USART_INT_GetUsart(USARTx);
	
	/* Check if we have any data in buffer */
	if (u->Num > 0) {
		if (u->Out == u->Size) {
			u->Out = 0;
		}
		c = *(u->Buffer + u->Out);
		u->Out++;
		u->Num--;
	}
	return c;
}

uint16_t USART_Gets(USART_TypeDef* USARTx, char* buffer, uint16_t bufsize) {
	uint16_t i = 0;
	
	/* Check for any data on USART */
	if (USART_BufferEmpty(USARTx) || (!USART_FindCharacter(USARTx, '\n') && !USART_BufferFull(USARTx))) {
		return 0;
	}
	
	/* If available buffer size is more than 0 characters */
	while (i < (bufsize - 1)) {
		/* We have available data */
		buffer[i] = (char) USART_Getc(USARTx);
		/* Check for end of string */
		if (buffer[i] == '\n') {
			i++;
			/* Done */
			break;
		} else {
			i++;
		}
	}
	
	/* Add zero to the end of string */
	buffer[i] = 0;               
	return (i);
}

uint8_t USART_BufferEmpty(USART_TypeDef* USARTx) {
	USART_t* u = USART_INT_GetUsart(USARTx);
	return (u->Num == 0);
}

uint8_t USART_BufferFull(USART_TypeDef* USARTx) {
	USART_t* u = USART_INT_GetUsart(USARTx);
	return (u->Num == u->Size);
}

void USART_ClearBuffer(USART_TypeDef* USARTx) {
	USART_t* u = USART_INT_GetUsart(USARTx);
	u->Num = 0;
	u->In = 0;
	u->Out = 0;
}

uint8_t USART_FindCharacter(USART_TypeDef* USARTx, volatile char c) {
	uint16_t num, out;
	USART_t* u = USART_INT_GetUsart(USARTx);
	
	/* Temp variables */
	num = u->Num;
	out = u->Out;
	
	while (num > 0) {
		/* Check overflow */
		if (out == u->Size) {
			out = 0;
		}
		if (u->Buffer[out] == c) {
			/* Character found */
			return 1;
		}
		out++;
		num--;
	}
	
	/* Character is not in buffer */
	return 0;
}

void USART_Puts(USART_TypeDef* USARTx, char* str) {
	while (*str) {
		/* Send char by char */
		USART_Putc(USARTx, *str++);
	}
}

void USART_Putc(USART_TypeDef* USARTx, volatile char c) {
	/* Wait to be ready */
	while (!USART_GetFlagStatus(USARTx, USART_FLAG_TXE));
	/* Send data */
	USARTx->DR = (uint16_t)(c & 0x01FF);
}

/* Private functions */
void USART_INT_InsertToBuffer(USART_t* u, uint8_t c) {
	/* Still available space in buffer */
	if (u->Num < u->Size) {
		/* Check overflow */
		if (u->In == u->Size) {
			u->In = 0;
		}
		/* Add to buffer */
		u->Buffer[u->In] = c;
		u->In++;
		u->Num++;
	}
}

USART_t* USART_INT_GetUsart(USART_TypeDef* USARTx) {
	USART_t* u;
	if (USARTx == USART2) {
		u = &F4_USART2;
	}
	return u;
}

uint8_t USART_INT_GetSubPriority(USART_TypeDef* USARTx) {
	uint8_t u;
	if (USARTx == USART2) {
		u = 4;
	}	
	return u;
}

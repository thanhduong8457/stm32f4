#include "stm32f4xx.h"
#include "system_timetick.h"

#define BUFF_SIZE 1000
#define BUFF_SIZE_Rx 4

uint8_t rxbuff[BUFF_SIZE_Rx];
uint16_t b[4*BUFF_SIZE_Rx];
uint16_t index2 = 0;
int T_m = 5;

volatile uint16_t ADCValue[3000]; 
void ADC_Config(void);

uint8_t txbuff[25] ;

void init_main(void);

float volt;
int volt1;
float volt_1;
float volt_2;

void DMA2_Stream0_IRQHandler(void);

uint16_t rcv_flag = 0;
uint16_t index1 = 0;
uint16_t a[9];
int k = 0;
float ADC_1=0, ADC_2=0, ADC_3=0;

int main(void) {
  /* Enable SysTick at 1ms interrupt */
  SysTick_Config(SystemCoreClock/1000);

  init_main();

  while(true){
    if(tick_count >= T_m){
      tick_count = 0; // phai du 3 buoc duoi cho DMA. 1 la xoa co, 2 la gan byte truyen, 3 la cho phep DMA bat dau truyen
      DMA_ClearFlag(DMA1_Stream4, DMA_FLAG_TCIF4); //moi lan DMA hoan thanh thi DMA_FLAG_TCIFx bat len 1, phai xoa co nay cho lan ke tiep
      DMA1_Stream4->NDTR = BUFF_SIZE; //muon truyen bao nhieu byte nap vo NDTR, vd 50 bytes, NDTR = 50;moi lan truyen xong 1 byte thi NDTR--
      DMA_Cmd(DMA1_Stream4, ENABLE); //bat dau cho DMA truyen
      DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
      GPIO_ToggleBits(GPIOD,GPIO_Pin_12);
    }
  }
}

void init_main(void) {
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); //Enable clock GPIOD
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); /* Enable GPIO clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE); /* Enable UART clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE); /* Enable DMA1 clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); /* Enable DMA2 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); /* Enable ADC1 clock */

  /* Connect UART4 pins to AF2 */  
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_UART4);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_UART4); 

  GPIO_InitTypeDef     GPIO_InitStructure; 

  /* GPIO Configuration for UART4 Tx */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* GPIO Configuration for USART Rx */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

  GPIO_Init(GPIOD, &GPIO_InitStructure);  
       
  /* USARTx configured as follow:
  - BaudRate = 115200 baud  
  - Word Length = 8 Bits
  - One Stop Bit
  - No parity
  - Hardware flow control disabled (RTS and CTS signals)
  - Receive and transmit enabled
  */
  USART_InitTypeDef USART_InitStructure;

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(UART4, &USART_InitStructure);
  USART_Cmd(UART4, ENABLE); /* Enable USART */
  USART_DMACmd(UART4, USART_DMAReq_Tx, ENABLE); /* Enable UART4 DMA */
    
  DMA_InitTypeDef DMA_InitStructure;
  /* DMA1 Stream4 Channel4 for UART4 Tx configuration */            
  DMA_InitStructure.DMA_Channel             = DMA_Channel_4;  
  DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)&UART4->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)txbuff;
  DMA_InitStructure.DMA_DIR                 = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_BufferSize          = BUFF_SIZE;
  DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode                = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority            = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;

  DMA_Init(DMA1_Stream4, &DMA_InitStructure);
  DMA_Cmd(DMA1_Stream4, ENABLE);
  USART_DMACmd(UART4, USART_DMAReq_Rx, ENABLE);
    
  /* DMA1 Stream2 Channel4 for USART4 Rx configuration */            
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;  
  DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)&UART4->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)rxbuff;
  DMA_InitStructure.DMA_DIR                 = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize          = BUFF_SIZE_Rx;
  DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode                = DMA_Mode_Normal;//DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority            = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;
  DMA_Init(DMA1_Stream2, &DMA_InitStructure);
  DMA_Cmd(DMA1_Stream2, ENABLE);

  NVIC_InitTypeDef  NVIC_InitStructure;

  /* Enable DMA Interrupt to the highest priority */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  DMA_ITConfig(DMA1_Stream2, DMA_IT_TC, ENABLE); /* Transfer complete interrupt mask */
    
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3| GPIO_Pin_5; ; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; /*choose mode analog*/ 
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; 
  GPIO_Init(GPIOC, &GPIO_InitStructure); 

  DMA_InitStructure.DMA_Channel = DMA_Channel_0;                                /*chanel duoc ho tro la chanel 0-do bang*/ 
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADCValue;                  /*ghep bien DMA_Memory0BaseAddr chua dia chi va cung kieu voi bien ADCValue*/ 
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(ADC1->DR));           /*gan dia chi thanh ghi chua gia tri chuyen doi ADC vao bien DMA_PeripheralBaseAddr cua DMA*/ 
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;                       /*chon huong chuyen du lieu*/ 
  DMA_InitStructure.DMA_BufferSize = 3000;                                      /*chon kich thuoc mang du lieu*/ 
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;              /*moi lan chuyen du lieu, dia chi ngoai vi se ko tang dan*/ 
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                       /*moi khi chuyen du lieu can tang dia chi bo nho*/ 
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;   /* kich thuoc thanh ghi chua du lieu ngoai vi la 16bit*/ 
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;           /*kich thuoc mang du lieu ADCValue là 16bit*/ 
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;                               /*chon mode DMA vong tron, viec chuyen doi lien tuc lap lai*/ 
  DMA_InitStructure.DMA_Priority = DMA_Priority_Low;                            /*thiet lap che do uu tien cao*/ 
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable; 
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull; 
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single; 
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; 
  DMA_Init(DMA2_Stream0, &DMA_InitStructure);                                   /* DMA2_Stream0 enable */ 
  DMA_Cmd(DMA2_Stream0, ENABLE); 

  NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;                       //Chu y trong startup_stm32f40_41.s dong 100
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
 
  ADC_CommonInitTypeDef  ADC_CommonInitStructure;
  //DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
  
  /* ADC Common Init 
  **********************************************************/ 
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;                      /*config mode 
  cho ADC*/ 
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;                   /*conduct devider by 2, for ADC take samples at highest frequency*/ 
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; 
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;  /*time between 2 times get sample*/ 
  ADC_CommonInit(&ADC_CommonInitStructure); 

  ADC_InitTypeDef ADC_InitStructure;

  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;                         /*ADC 12 bits resolution*/ 
  ADC_InitStructure.ADC_ScanConvMode = ENABLE; 
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; 
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; 
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; 
  ADC_InitStructure.ADC_NbrOfConversion = 3;                                     /*the number of chanel ADC converter*/ 
  ADC_Init(ADC1, &ADC_InitStructure); 

  /* ADC1 regular channels configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 2, ADC_SampleTime_56Cycles); 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 1, ADC_SampleTime_56Cycles); 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_15, 3, ADC_SampleTime_56Cycles); 

  /* Enable ADC1 DMA */ 
  ADC_DMACmd(ADC1, ENABLE); 
  /* Enable DMA request after last transfer (Single-ADC mode) */ 
  ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE); 

  /* Enable ADC1 */ 
  ADC_Cmd(ADC1, ENABLE); 
  
  /* Start ADC1 Software Conversion */ 
  ADC_SoftwareStartConv(ADC1); 
}

//dat dung ten thi khi co ngat no nhay vao
void DMA2_Stream0_IRQHandler(void) {
  uint16_t i, m=0, k=0, f=0;
    
  /* Clear the DMA1_Stream2 TCIF2 pending bit */
  DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF2); // clear interrupt flag*/

  //handle data
  for(i=0; i<2998; i=i+3) {
    ADC_1 = ADC_1 + ADCValue[i];
    k = i + 1;
    ADC_2 = ADC_2 + ADCValue[k];
    m = i + 2;
    ADC_3 = ADC_3 + ADCValue[m];
  }// copy 4 bytea to a

  volt=(ADC_1/1000)*3.0/4095.0;

  txbuff[0]='|';
  txbuff[1]=0x0D;
  txbuff[2]=(int)(volt)+48;
  txbuff[3]=',';
  txbuff[4]=(int)((volt-txbuff[2]+48)*10)+48;
  txbuff[5]=(int)(volt*100-(txbuff[2]-48)*100-(txbuff[4]-48)*10)+48;
  txbuff[6]='|';        
  txbuff[7]=0x0D;

  volt_1=(ADC_2/1000)*3.0/4095.0;

  txbuff[8]=(int)(volt_1)+48;
  txbuff[9]=',';
  txbuff[10]=(int)((volt_1-txbuff[8]+48)*10)+48;
  txbuff[11]=(int)(volt_1*100-(txbuff[8]-48)*100-(txbuff[10]-48)*10)+48;
  txbuff[12]='|';        
  txbuff[13]=0x0D;

  volt_2=(ADC_3/1000)*3.0/4095.0;

  txbuff[14]=(int)(volt_2)+48;
  txbuff[15]=',';
  txbuff[16]=(int)((volt_2-txbuff[14]+48)*10)+48;
  txbuff[17]=(int)(volt_2*100-(txbuff[14]-48)*100-(txbuff[16]-48)*10)+48;
  txbuff[18]='|';        
  txbuff[19]=0x0D;
  txbuff[20]=T_m/1000+48; 
  txbuff[21]=(T_m -(txbuff[20]-48)*1000)/100 + 48;
  txbuff[22]=(T_m - (txbuff[20]-48)*1000 - (txbuff[21]-48)*100)/10 + 48;
  txbuff[23]=(T_m - (txbuff[20]-48)*1000 - (txbuff[21]-48)*100)%10 + 48;
  txbuff[24]='\n';
        
  rcv_flag = 1;
  k = k + 1;
  //enable DMA --> alway available
  ADC_1 = 0;
  ADC_2 = 0;
  ADC_3 = 0;
  DMA_Cmd(DMA2_Stream0, ENABLE); 
  DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, DISABLE);
}

void DMA1_Stream2_IRQHandler(void) {
  uint16_t i;
  /* Clear the DMA1_Stream2 TCIF2 pending bit */
  DMA_ClearITPendingBit(DMA1_Stream2, DMA_IT_TCIF2);

  for(i=0; i<BUFF_SIZE_Rx; i++) {
    rxbuff[i] = rxbuff[i]-0x30;
  }

  T_m = rxbuff[0]*1000 + rxbuff[1]*100 + rxbuff[2]*10 + rxbuff[3];

  DMA_Cmd(DMA1_Stream2, ENABLE);
}

#include <iostream>
#include <string>

#include "stm32f4xx.h"
// #include "system_timetick.h"
// #include "stdio.h"

// void init_gpio(void);

void CAN_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIO clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    /* Configure CAN RX and TX pins */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Connect CAN pins to AF9 */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_CAN1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_CAN1);
}

void CAN_Config(void) {
    CAN_InitTypeDef CAN_InitStructure;
    CAN_FilterInitTypeDef CAN_FilterInitStructure;

    /* Enable CAN clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    /* CAN register init */
    CAN_DeInit(CAN1);

    /* CAN cell init */
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1 = CAN_BS1_9tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_4tq;
    CAN_InitStructure.CAN_Prescaler = 4;
    CAN_Init(CAN1, &CAN_InitStructure);

    /* CAN filter init */
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
}

void CAN_NVIC_Config(void) {
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable FIFO 0 message pending Interrupt */
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

void CAN1_RX0_IRQHandler(void) {
    CanRxMsg RxMessage;
    if (CAN_GetITStatus(CAN1, CAN_IT_FMP0) != RESET) {
        CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
        CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);

        /* Process received message */
    }
}

void CAN_Send_Message(uint32_t id, uint8_t* data, uint8_t length) {
    CanTxMsg TxMessage;
    uint8_t i;

    TxMessage.StdId = id;
    TxMessage.RTR = CAN_RTR_DATA;
    TxMessage.IDE = CAN_ID_STD;
    TxMessage.DLC = length;

    for (i = 0; i < length; i++) {
        TxMessage.Data[i] = data[i];
    }

    CAN_Transmit(CAN1, &TxMessage);
}

void CAN_Receive_Message(void) {
    CanRxMsg RxMessage;

    if (CAN_MessagePending(CAN1, CAN_FIFO0) != 0) {
        CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);

        /* Process received message */
    }
}

int main(void) {
    // delay_init();
    // init_gpio();

    SystemInit();
    CAN_GPIO_Init();
    CAN_Config();
    CAN_NVIC_Config();

    uint8_t message[8] = "Hello";
    CAN_Send_Message(0x321, message, 5);

    while (1) {
        CAN_Receive_Message();
    }
}

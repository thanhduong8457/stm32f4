#pragma once

#include <stdio.h>

extern int share_value;

void UART1_Init(void);
void UART1_SendChar(char ch);

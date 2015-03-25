/**
  * @file    driver/uart_drv.h
  * @brief   Uart Driver for Debug Print
  */
#ifndef _USART_DRV_H_
#define _USART_DRV_H_
#include "stm32f10x.h"

#define USE_INTERRUPT_USART
#define DEBUG_PORT  UART_PORT_2

typedef enum
{
	UART_PORT_1,
	UART_PORT_2,
	UART_PORT_3,
	UART_PORT_MAX
} UART_PORT;

#define QUEUE_SIZE	0x400

/**
  * @brief  Default Configuration for USART1
  * @param  None
  * @retval None
  */
void USART_Configuration(UART_PORT port, int baudrate, int bEnableIntr);

char SerialGetKey(UART_PORT port);
int SerialGetChar(UART_PORT port, char *key);
int SerialGetCharfromQ(UART_PORT port, char *key);
void Serial_PutCharCR(UART_PORT port, char data);
void Serial_PutString(UART_PORT port, u8* data);

#endif

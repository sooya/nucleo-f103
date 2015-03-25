/**
  * @file    driver/uart_drv.c
  * @brief   Uart Driver for Debug Print
  */

#include "stm32f10x.h"

#include "uart_drv.h"

#define RX_QUEUE_SIZE	(QUEUE_SIZE+0x10)

static char uartRxQueue[UART_PORT_MAX][RX_QUEUE_SIZE];
static int rIdx[UART_PORT_MAX] = {0, };
static int wIdx[UART_PORT_MAX] = {0, };

static int UARTQ_IsEmpty(UART_PORT port);
static int UARTQ_UpdateRdPtr(UART_PORT port, int length);
static int UARTQ_PushData(UART_PORT port, char rxData);

static char *UARTQ_GetData(UART_PORT port, int *length);
static int UARTQ_GetByte(UART_PORT port, char *c);

/////////////////////////////////////////////////////////////////////////////
// LED Function
static int ledtoggle[UART_PORT_MAX] = {0, };

void OnBoardLED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// For LED
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void OnBoardLED_OnOff(int onoff)
{
    if(onoff%2 ==0)
        GPIOA->BRR = GPIO_Pin_5;
    else
        GPIOA->BSRR = GPIO_Pin_5;
}

/////////////////////////////////////////////////////////////////////////////
// UART Function
/**
  * @brief  Default Configuration for USART1
  * @param  None
  * @retval None
  */
void USART_Configuration(UART_PORT port, int baudrate, int bEnableIntr)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	USART_TypeDef *pPort = 0;
#ifdef USE_INTERRUPT_USART
	NVIC_InitTypeDef NVIC_InitStructure;
#endif

    OnBoardLED_Init();

	switch(port)
	{
	case UART_PORT_1:
		// RCC Configuration for USART1
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA , ENABLE );

		/* Configure USART1 Tx as alternate function push-pull */
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		/* Configure USART2 Rx as input floating */
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

#ifdef USE_INTERRUPT_USART
		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
#endif
		pPort = USART1;
		break;

	case UART_PORT_2:
		// RCC Configuration for USART1
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE );
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE );

		/* Configure USART1 Tx as alternate function push-pull */
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		/* Configure USART2 Rx as input floating */
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

#ifdef USE_INTERRUPT_USART
		NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
#endif
		pPort = USART2;
		break;

	case UART_PORT_3:
		// RCC Configuration for USART3
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE );
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE );
		/* Configure USART3 Tx as alternate function push-pull */
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOB, &GPIO_InitStructure);

		/* Configure USART2 Rx as input floating */
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
		GPIO_Init(GPIOB, &GPIO_InitStructure);

#ifdef USE_INTERRUPT_USART
		NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
#endif
		pPort = USART3;
		break;
	default:
		pPort = 0;
		return;
	}


	USART_InitStructure.USART_BaudRate				= baudrate;
	USART_InitStructure.USART_WordLength			= USART_WordLength_8b;
	USART_InitStructure.USART_StopBits				= USART_StopBits_1;
	USART_InitStructure.USART_Parity				= USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode				= USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(pPort, &USART_InitStructure);

#ifdef USE_INTERRUPT_USART
	USART_ITConfig(pPort, USART_IT_RXNE, bEnableIntr);

	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = bEnableIntr;
	NVIC_Init(&NVIC_InitStructure);
#endif
//	USART_SetIntr(port, bEnableIntr);

	USART_Cmd(pPort, ENABLE);
}


/*!
  @brief	send byte though UART
  @param	[in] data			send data.
  @return	1 means success send data. \n
 */
int UART_PutChar(UART_PORT port, u8 c)
{
	int ret = 0;
	USART_TypeDef *pPort = 0;

	switch(port)
	{
	case UART_PORT_1:
		pPort = USART1;
		break;
	case UART_PORT_2:
		pPort = USART2;
		break;
	case UART_PORT_3:
		pPort = USART3;
		break;
	default:
		pPort = 0;
		return;
	}

	if(USART_GetFlagStatus(pPort, USART_FLAG_TXE) == RESET)
	{
		ret = 0;
	}
	else
	{
		USART_SendData(pPort, c);
		ret = 1;
	}

	return ret;
}

/*!
  @brief	receive byte though UART
  @param	[out] pdata			receive data.
  @return	1 means success receive data. \n
 */
int UART_GetChar(UART_PORT port, char *pdata)
{
	int ret = 0;
	USART_TypeDef *pPort = 0;

	switch(port)
	{
	case UART_PORT_1:
		pPort = USART1;
		break;
	case UART_PORT_2:
		pPort = USART2;
		break;
	case UART_PORT_3:
		pPort = USART3;
		break;
	default:
		pPort = 0;
		return;
	}

	if ( USART_GetFlagStatus(pPort, USART_FLAG_RXNE) != RESET)
	{
		*pdata = (u8)pPort->DR;
		ret = 1;
	}
	else
	{
	    OnBoardLED_OnOff(ledtoggle[port]++);
		ret = 0;
	}

	return ret;
}

void Serial_PutChar(UART_PORT port, char data)
{
	while(!UART_PutChar(port, data));
}

void Serial_PutCharCR(UART_PORT port, char data)
{
	if(data==0x0d)			// '\n'
		Serial_PutChar(port, 0x0a);	// '\r'
	Serial_PutChar(port, data);
}

char SerialGetKey(UART_PORT port)
{
	char key = 0;
	while(!UART_GetChar(port, &key));
	return key;
}

int SerialGetChar(UART_PORT port, char *pdata)
{
	int ret;
	ret = UART_GetChar(port, pdata);
	return ret;
}


int SerialGetCharfromQ(UART_PORT port, char *key)
{
	int ret;

	ret = UARTQ_GetByte(port, key);

	return ret;
}

void Serial_PutStringLen(UART_PORT port, u8* data,u16 len)
{
	u16 i;
	for (i=0; i<len; i++){
		Serial_PutChar(port, data[i]);
	}
}

void Serial_PutString(UART_PORT port, u8* data)
{
	while(*data != 0)
	{
		Serial_PutChar(port, *data);
		data++;
	}
}




/////////////////////////////////////////////////////////////////////////////////
//
int UARTQ_IsEmpty(UART_PORT port)
{
	int ret = 0;
	if(rIdx[port]==wIdx[port])
		ret = 1;

	return ret;
}

int UARTQ_GetByte(UART_PORT port, char *c)
{
	int packet_length;
	unsigned char *packet = 0;//NULL;

	packet = (unsigned char *) UARTQ_GetData(port, &packet_length);
	if(packet_length == 0)
	{
		//Empty UART Q
		return -1;
	}

	*c = packet[0];
	UARTQ_UpdateRdPtr(port, 1);

	return 0;
}

char *UARTQ_GetData(UART_PORT port, int *length)
{
	int bufsize = 0;
	char *packet = uartRxQueue[port];

	if(rIdx[port]==wIdx[port])
	{
		bufsize = 0;
	}
	else
	{
		if(rIdx[port] < wIdx[port])
		{
			bufsize = wIdx[port] - rIdx[port];
		}
		else
		{
			bufsize = RX_QUEUE_SIZE - rIdx[port];
		}

		packet += rIdx[port];
	}

	*length = bufsize;

	return packet;
}

int UARTQ_UpdateRdPtr(UART_PORT port, int length)
{
	int ret = 0;
	int nextRIdx = rIdx[port] + length;
	if(nextRIdx > RX_QUEUE_SIZE)
	{
//		printf("UARTQ_UpdateRdPtr Error!!!\n");
		ret = -1;
	}
	else if(nextRIdx == RX_QUEUE_SIZE)
	{
		nextRIdx = 0;
	}

	rIdx[port] = nextRIdx;
	return ret;
}

int UARTQ_PushData(UART_PORT port, char rxData)
{
	int ret =0;
	int nextWIdx;

	nextWIdx = (wIdx[port] + 1)%RX_QUEUE_SIZE;
	if(nextWIdx == rIdx[port])
	{
//		printf("UARTQ buffer Full %d, %d\r\n", rIdx, wIdx);
		ret = -1;
	}
	else
	{
		uartRxQueue[port][wIdx[port]] = rxData;
		wIdx[port] = nextWIdx;
	}

	return ret;
}

#ifdef USE_INTERRUPT_USART
void USART1_IRQHandler(void)
{
	char rxData;
	int rxEmpty = 0;
	int count = 0;

//	LED3_OnOff(ledtoggle[UART_PORT_1]++);

	while (!rxEmpty)
	{
		if (UART_GetChar(UART_PORT_1, &rxData) == 0)
		{
			if(count == 0x1000)
				rxEmpty = 1;
			count++;
		}
		else
		{
			UARTQ_PushData(UART_PORT_1, rxData);
		}
	}
}

void USART2_IRQHandler(void)
{
	char rxData;
	int rxEmpty = 0;
	int count = 0;

//	LED3_OnOff(ledtoggle[UART_PORT_2]++);

	while (!rxEmpty)
	{
		if (UART_GetChar(UART_PORT_2, &rxData) == 0)
		{
			if(count == 0x1000)
				rxEmpty = 1;
			count++;
		}
		else
		{
			UARTQ_PushData(UART_PORT_2, rxData);
		}
	}
}

void USART3_IRQHandler(void)
{
	char rxData;
	int rxEmpty = 0;
	int count = 0;

//	LED4_OnOff(ledtoggle[UART_PORT_3]++);

	while (!rxEmpty)
	{
		if (UART_GetChar(UART_PORT_3, &rxData) == 0)
		{
			if(count == 0x1000)
				rxEmpty = 1;
			count++;
		}
		else
		{
			UARTQ_PushData(UART_PORT_3, rxData);
		}
	}
}
#endif

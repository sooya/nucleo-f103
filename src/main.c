/*
**
**                           Main.c
**
**
**********************************************************************/
/*
   Last committed:     $Revision: 00 $
   Last changed by:    $Author: $
   Last changed date:  $Date:  $
   ID:                 $Id:  $

**********************************************************************/
#include "stm32f10x.h"
//#include "stdio.h"
#include "uart_drv.h"

//////////////////////////////////////////////////////////////////////////////////
void print_clk(void)
{
	RCC_ClocksTypeDef RCC_ClocksStatus;
	RCC_GetClocksFreq(&RCC_ClocksStatus);
	cm_printf("SYSCLK : %d\r\n", RCC_ClocksStatus.SYSCLK_Frequency);
	cm_printf("HCLK : %d\r\n", RCC_ClocksStatus.HCLK_Frequency);
	cm_printf("PCLK1 : %d\r\n", RCC_ClocksStatus.PCLK1_Frequency);
	cm_printf("PCLK2 : %d\r\n", RCC_ClocksStatus.PCLK2_Frequency);
	cm_printf("ADCCLK : %d\r\n", RCC_ClocksStatus.ADCCLK_Frequency);
}

int main(void)
{
    char c;
    int bInterrupt = 0;

#ifdef USE_INTERRUPT_USART
    bInterrupt = 1;
#endif
    USART_Configuration(DEBUG_PORT, 115200, bInterrupt);
	cm_printf("\r\n");
	print_clk();
	cm_printf("start application\r\n");
	Serial_PutCharCR(DEBUG_PORT, '#');
	Serial_PutCharCR(DEBUG_PORT, ' ');

	do {
#ifdef USE_INTERRUPT_USART
		while(SerialGetCharfromQ(DEBUG_PORT, &c)==0)
#else
		if(SerialGetChar(DEBUG_PORT, &c)==1)
#endif
		{
			Serial_PutCharCR(DEBUG_PORT, c);
			// Parse Command

			if(c==0x0d)
			{
				Serial_PutCharCR(DEBUG_PORT, '#');
				Serial_PutCharCR(DEBUG_PORT, ' ');
			}
		}
	} while (1);
}

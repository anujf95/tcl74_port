/*
 * uart_stream.c
 *
 *  Created on: 13-Mar-2020
 *      Author: anuj
 */

#include "uart_stream.h"

#include "inc/tm4c123gh6pm.h"
#include "uart.h"
#include "global.h"

#include <stdio.h>

int tcl_uart_async_handler(void);

static void __attribute__((constructor)) uart_stream_setup(void)
{
    uart0_init(115200,UART_LC_8_BIT | UART_LC_1_STOP_BIT | UART_LC_NO_PARITY,0);
}



int uart_stream_write(char *ptr, int len)
{
    uint32_t cnt;
    for (cnt = 0; cnt < len; cnt++)
    {
        uart0_write(*ptr++);
    }
    return cnt;
}

int uart_stream_read(char *ptr, int len)
{
    uint32_t cnt=0;
    char ch;
    while (cnt < len)
    {
        while(check(UART0_FR_R,4))
		{	
			tcl_uart_async_handler();
		}
		ch = UART0_DR_R;
		
		if(ch=='\r')
			ch='\n';
		
#ifdef UART_STREAM_LOCAL_ECHO
        uart0_write(ch);
#endif
		
		switch(ch)
		{
			case '\r':
				ch = '\n';
			case '\n':
				ptr[cnt++]='\n';
				return cnt;
			
			case '@':
				ptr[cnt++]=EOF;
				return cnt;
			
			case '\b':
				#ifdef UART_STREAM_LOCAL_ECHO
					uart0_write(' ');
					uart0_write('\b');
				#endif
				--cnt;
				break;
			
			default:
				ptr[cnt++]=ch;
				break;
		}       
        
    }
    return len;
}

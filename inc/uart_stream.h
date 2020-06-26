/*
 * uart_stream.h
 *
 *  Created on: 13-Mar-2020
 *      Author: anuj
 */

#ifndef UART_STREAM_H_
#define UART_STREAM_H_

#include <stdint.h>
#include <stdbool.h>

#define UART_STREAM_BLOCKING 1  //UART_STREAM as BLOCKING or BUFFERED
#define UART_STREAM_INSTANCE UART0_BASE
#define UART_STREAM_LOCAL_ECHO

int uart_stream_write(char *ptr, int len);
int uart_stream_read(char *ptr, int len);

#endif /* UART_STREAM_H_ */

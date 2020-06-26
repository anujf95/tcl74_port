/*
 * uart.h
 *
 *  Created on: 19-Feb-2020
 *      Author: anuj
 */

#ifndef TIVA_LIB_UART_H_
#define TIVA_LIB_UART_H_

#include <stdint.h>
#include <inc/tm4c123gh6pm.h>

#define UART_CLK (F_CPU/16)

#define UART_LC_5_BIT (0x00)
#define UART_LC_6_BIT (0x20)
#define UART_LC_7_BIT (0x40)
#define UART_LC_8_BIT (0x60)
#define UART_LC_FIFO_EN (0x10)
#define UART_LC_1_STOP_BIT (0x00)
#define UART_LC_2_STOP_BIT (0x07)
#define UART_LC_EVEN_PARITY (0x03)
#define UART_LC_ODD_PARITY (0x02)
#define UART_LC_NO_PARITY (0x00)


void uart0_init(uint32_t baud_rate,uint32_t line_config, uint8_t rx_int_en);
uint8_t uart0_read(void);
void uart0_write(uint8_t data);
void uart0_string(char *str);

void uart2_init(uint32_t baud_rate,uint32_t line_config, uint8_t rx_int_en);
uint8_t uart2_read(void);
void uart2_write(uint8_t data);

#endif /* TIVA_LIB_UART_H_ */

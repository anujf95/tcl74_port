/*
 * uart.c
 *
 *  Created on: 19-Feb-2020
 *      Author: anuj
 */
#include "uart.h"
#include "inc/tm4c123gh6pm.h"
#include "global.h"

void uart0_init(uint32_t baud_rate,uint32_t line_config, uint8_t rx_int_en)
{

    //Provide clock to UART0 by writing a 1 to RCGCUART register.
    SYSCTL_RCGC1_R |= 0x01;

    //Provide clock to PORTA by writing a 1 to RCGCGPIO register.
    SYSCTL_RCGC2_R |= 0x01;

    //Disable the UART0 by writing 0 to UARTCTL register of UART0.
    cbit(UART0_CTL_R,0);

    //Write the integer portion of the Baud rate to the UARTIBRD register of UART0.
    UART0_IBRD_R = (UART_CLK/baud_rate);

    //Write the fractional portion of the Baud rate to the UARTFBRD register of UART0.
    UART0_FBRD_R = ( ((64*UART_CLK)+(baud_rate/2))/baud_rate) - (64*UART0_IBRD_R);

    //Select the system clock as UART clock source by writing a 0 to UARTCC register of UART0.
    UART0_CC_R = 0;

    //Configure the line control value
    UART0_LCRH_R = line_config;

    //Set TxE and RxE bits in UARTCTL register to enable the transmitter and receiver of UART0.
    UART0_CTL_R |= 0x0300;

    //enable RX interrupt if configured
    if(rx_int_en)
        UART0_IM_R |= 0x0010;   //RXIM=1


    //Set UARTEN bit in UARTCTL register to enable the UART0.
    UART0_CTL_R |= 0x01;

    //Make PA0 and PA1 pins to be used as Digital I/O.
    GPIO_PORTA_DEN_R |= 0x03;

    //Select the alternate functions of PA0 (RxD) and PA1 (TxD) pins using the GPIOAFSEL.
    GPIO_PORTA_AFSEL_R |= 0x03;

    //Configure PA0 and PA1 pins for UART function.
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) | 0x00000011;

    //Wait for TxD output to establish idle high.
    //while(!check(GPIO_PORTA_DATA_R,1));

}

uint8_t uart0_read(void)
{
    char data;
    //Monitor the RXFE flag bit in UART Flag register and when it goes LOW (buffer not empty),
    //read the received byte from Data register and save it.
    while(check(UART0_FR_R,4));
    data = UART0_DR_R;
    return data;
}

void uart0_write(uint8_t data)
{
    //Monitor the TXFF flag bit in UART Flag register and when it goes LOW (buffer not full),
    //write a byte to Data register to be transmitted.
    while(!check(UART0_FR_R,7));
    UART0_DR_R = data;
}

void uart0_string(char *str)
{
    while(*str != '\0')
        uart0_write(*str++);
}

void uart2_init(uint32_t baud_rate,uint32_t line_config, uint8_t rx_int_en)
{

    //Provide clock to UART0 by writing a 1 to RCGCUART register.
    SYSCTL_RCGC1_R |= 0x04;

    //Provide clock to PORTD by writing a 1 to RCGCGPIO register.
    SYSCTL_RCGC2_R |= 0x08;

    //Disable the UART0 by writing 0 to UARTCTL register of UART0.
    cbit(UART2_CTL_R,0);

    //Write the integer portion of the Baud rate to the UARTIBRD register of UART0.
    UART2_IBRD_R = (UART_CLK/baud_rate);

    //Write the fractional portion of the Baud rate to the UARTFBRD register of UART0.
    UART2_FBRD_R = ( ((64*UART_CLK)+(baud_rate/2))/baud_rate) - (64*UART2_IBRD_R);

    //Select the system clock as UART clock source by writing a 0 to UARTCC register of UART0.
    UART2_CC_R = 0;

    //Configure the line control value
    UART2_LCRH_R = line_config;

    //Set TxE and RxE bits in UARTCTL register to enable the transmitter and receiver of UART0.
    UART2_CTL_R |= 0x0300;

    //enable RX interrupt if configured
    if(rx_int_en)
        UART2_IM_R |= 0x0010;   //RXIM=1

    //Set UARTEN bit in UARTCTL register to enable the UART0.
    UART2_CTL_R |= 0x01;

    //Make PD6 and PD7 pins to be used as Digital I/O.
    GPIO_PORTD_LOCK_R = 0x4C4F434B; /* unlock GPIO Port D */
    GPIO_PORTD_CR_R = 0x80;         /* allow changes to PD7*/
    GPIO_PORTD_DEN_R |= 0xC0;

    //Select the alternate functions of PA0 (RxD) and PA1 (TxD) pins using the GPIOAFSEL.
    GPIO_PORTD_AFSEL_R |= 0xC0;

    //Configure PD6 and PD7 pins for UART function.
    GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0x00FFFFFFFF) | 0x11000000;

    //Wait for TxD output to establish idle high. TxD=PD7
    //while(!check(GPIO_PORTD_DATA_R,7));

}

uint8_t uart2_read(void)
{
    char data;
    //Monitor the RXFE flag bit in UART Flag register and when it goes LOW (buffer not empty),
    //read the received byte from Data register and save it.
    while(check(UART2_FR_R,4));
    data = UART2_DR_R;
    return data;
}

void uart2_write(uint8_t data)
{
    //Monitor the TXFF flag bit in UART Flag register and when it goes LOW (buffer not full),
    //write a byte to Data register to be transmitted.
    while(!check(UART2_FR_R,7));
    UART2_DR_R = data;
}


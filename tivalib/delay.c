/*
 * delay.c
 *
 *  Created on: 29-Jan-2020
 *      Author: anuj
 */

#include "delay.h"

void delay_ms(uint32_t ms)
{
    for(uint32_t i=0; i<ms; i++)
        for (volatile uint32_t i=0; i<1580; i++)
            __asm("NOP");
}

void delay_us(uint32_t us)
{
    for(volatile uint32_t i=0; i<us; i++)
        __asm("NOP");
}

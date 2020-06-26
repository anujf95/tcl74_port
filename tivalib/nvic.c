/*
 * intr.c
 *
 *  Created on: 12-Feb-2020
 *      Author: anuj
 */
#include "nvic.h"
#include "gpio.h"
#include "inc/tm4c123gh6pm.h"

void nvic_enable_intr(void)
{
    __asm("CPSIE  I");    //clear PRIMASK
    __asm("CPSIE  F");    //clear FAULTMASK
}

void nvic_disable_intr(void)
{
    __asm("CPSID  I");     //set PRIMASK
    //__asm ("CPSID  F");   //set FAULTMASK
}

void nvic_set_priority(volatile uint32_t *reg, int byte, uint8_t priority)
{
    //3 bit priority field
    uint32_t clearmask = ~(7<<(byte*8+5));
    *reg = (*reg & clearmask) | (priority<<(byte*8+5));
}

void nvic_enable_irq(uint8_t irq)
{
    switch(irq/32)
    {
        case 0:
            sbit(NVIC_EN0_R,irq%32);
            break;
        case 1:
            sbit(NVIC_EN1_R,irq%32);
            break;
        case 2:
            sbit(NVIC_EN2_R,irq%32);
            break;
        case 3:
            sbit(NVIC_EN3_R,irq%32);
            break;
        case 4:
            sbit(NVIC_EN4_R,irq%32);
            break;
        default:
            while(1);
            break;
    }
}

/*
 * gpio.c
 *
 *  Created on: 22-Jan-2020
 *      Author: anuj
 */

#include "gpio.h"


void gpio_clk_init(void)
{
    //enable clock for all port
    SYSCTL_RCGC2_R |= 0x0000003F;
}

void gpio_portf_enable(void)
{
    SYSCTL_RCGC2_R |= 0x00000020;   /* 1) activate clock for Port F */

    GPIO_PORTF_LOCK_R = 0x4C4F434B; /* 2) unlock GPIO Port F */
    GPIO_PORTF_CR_R = 0x1F;         /* allow changes to PF4-0 */
    GPIO_PORTF_AMSEL_R = 0x00;      /* 3) disable analog on PF */
    GPIO_PORTF_PCTL_R = 0x00000000; /* 4) PCTL GPIO on PF4-0 */
    GPIO_PORTF_DIR_R = 0x0E;        /* 5) PF4,PF0 in, PF3-1 out */
    GPIO_PORTF_AFSEL_R = 0x00;      /* 6) disable alt funct on PF7-0 */
    GPIO_PORTF_PUR_R = 0x11;        /* enable pull-up on PF0 and PF4 */
    GPIO_PORTF_DEN_R = 0x1F;        /* 7) enable digital I/O on PF4-0 */
}

void gpio_portf_intr_enable(int pin, int level, int dual_edge, int rising_edge)
{
    if(level)
        sbit(GPIO_PORTF_IS_R,pin);  //set as level triggered
    else
        cbit(GPIO_PORTF_IS_R,pin);  //set as edge triggered

    if(dual_edge)
        sbit(GPIO_PORTF_IBE_R,pin); //set as intr on both edges
    else
        cbit(GPIO_PORTF_IBE_R,pin); //set as intr on both edges

    if(rising_edge)
        sbit(GPIO_PORTF_IEV_R,pin); //set as intr on rising edge
    else
        cbit(GPIO_PORTF_IEV_R,pin); //set as intr on falling edge

    //enable in interrupt for pin
    sbit(GPIO_PORTF_IM_R,pin);

    //clear interrupt flag if already set
    sbit(GPIO_PORTF_ICR_R,pin);
}

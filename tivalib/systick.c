/*
 * systick.c
 *
 *  Created on: 13-Feb-2020
 *      Author: anuj
 */

#include "systick.h"

void systick_init(uint32_t freq_hz)
{
    //set reload register = F_CPU/frq_hz - 1
    NVIC_ST_RELOAD_R = (F_CPU/freq_hz) - 1;

    //clear systick count if already existing
    NVIC_ST_CURRENT_R = 0;

    /*Enable Systick with
        1. System Clock (bit 2) = 1
        2. Interrupt Enable (bit 1) = 1
        3. Enable (bit 0) = 1 */
    NVIC_ST_CTRL_R |= 0x7;
}

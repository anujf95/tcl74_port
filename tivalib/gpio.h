/*
 * gpio.h
 *
 *  Created on: 22-Jan-2020
 *      Author: anuj
 */

#ifndef INC_GPIO_H_
#define INC_GPIO_H_

#include <stdint.h>
#include "global.h"
#include "inc/tm4c123gh6pm.h"


/* For TIVA */
#define SW1_BIT (4)
#define SW2_BIT (0)
#define LED_R_BIT (1)
#define LED_B_BIT (2)
#define LED_G_BIT (3)

void gpio_clk_init(void);
void gpio_portf_enable(void);
void gpio_portf_intr_enable(int pin, int level, int dual_edge, int rising_edge);

#endif /* INC_GPIO_H_ */

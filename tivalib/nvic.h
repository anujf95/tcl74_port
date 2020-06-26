/*
 * nvic.h
 *
 *  Created on: 12-Feb-2020
 *      Author: anuj
 */

#ifndef INC_NVIC_H_
#define INC_NVIC_H_

#include <stdint.h>

void nvic_enable_intr(void);
void nvic_disable_intr(void);
void nvic_set_priority(volatile uint32_t *reg, int byte, uint8_t priority);
void nvic_enable_irq(uint8_t irq);

#endif /* INC_NVIC_H_ */

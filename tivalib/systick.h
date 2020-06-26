/*
 * systick.h
 *
 *  Created on: 13-Feb-2020
 *      Author: anuj
 */

#ifndef INC_SYSTICK_H_
#define INC_SYSTICK_H_

#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

void systick_init(uint32_t freq_hz);

#endif /* INC_SYSTICK_H_ */

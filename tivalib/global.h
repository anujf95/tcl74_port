/*
 * global.h
 *
 *  Created on: 19-Feb-2020
 *      Author: anuj
 */

#ifndef TIVA_LIB_GLOBAL_H_
#define TIVA_LIB_GLOBAL_H_

#include <stdint.h>

//set 'bit'th bit in 'reg'
#define sbit(reg,bit)   reg |=  (1<<(bit))

//clear 'bit'th bit in 'reg'
#define cbit(reg,bit)   reg &= ~(1<<(bit))

//toggle 'bit'th bit in 'reg'
#define tbit(reg,bit)   reg ^=  (1<<(bit))

//check 'bit'th bit in 'reg'
//bit=0 => return=(0) 
//bit=1 => return=(1<<bit)
#define check(reg,bit)  ( (reg) & (1<<(bit)) )

//modify 'reg' with 'mask' by 'value'
#define mreg(reg,mask,value)    reg = ((reg) & ~(mask)) | ( (value) & (mask) )

//Replace 'width' no. of bits starting from 'start'th bit with 'value' in 'reg'
//#define modreg(reg,start,width,value)   ((reg) & ~(((1<<width)-1)<<start)) | ( (value & ((1<<width)-1) )<<start )

#endif /* TIVA_LIB_GLOBAL_H_ */

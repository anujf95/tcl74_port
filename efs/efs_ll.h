#ifndef __EFS_LL_H__
#define __EFS_LL_H__

#include <stdint.h>

int efs_ll_write(uint8_t *data,uint32_t mem_addr,uint16_t size);
int efs_ll_read(uint8_t *data,uint32_t mem_addr,uint16_t size);
int efs_ll_fill(uint8_t data,uint32_t mem_addr,uint16_t size);

#endif
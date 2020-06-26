#include "efs_ll.h"


#define EMULATED_EEPROM

#if defined(EMULATED_EEPROM)
#define EEPROM_SIZE 2048
volatile uint8_t eeprom[EEPROM_SIZE];

int efs_ll_write(uint8_t *data,uint32_t mem_addr,uint16_t size)
{
    uint32_t cnt;
    for(cnt=0; cnt<size;cnt++)
    {
        if((mem_addr+cnt)>=EEPROM_SIZE)
            return cnt;
        else
            eeprom[mem_addr+cnt] = data[cnt];
    }
    return cnt;
}

int efs_ll_fill(uint8_t data,uint32_t mem_addr,uint16_t size)
{
    uint32_t cnt;
    for(cnt=0; cnt<size;cnt++)
    {
        if((mem_addr+cnt)>=EEPROM_SIZE)
            return cnt;
        else
            eeprom[mem_addr+cnt] = data;
    }
    return cnt;
}

int efs_ll_read(uint8_t *data,uint32_t mem_addr,uint16_t size)
{
    uint32_t cnt;
    for(cnt=0; cnt<size;cnt++)
    {
        if((mem_addr+cnt)>=EEPROM_SIZE)
            return cnt;
        else
            data[cnt] = eeprom[mem_addr+cnt];
    }
    return cnt;
}

#endif
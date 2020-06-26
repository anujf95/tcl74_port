#ifndef __EFS_MACRO_H__
#define __EFS_MACRO_H__

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "efs_config.h"
#include "efs_spec.h"


typedef struct EfsFd
{
    uint8_t InUse;
    uint8_t FileIndex;
    uint8_t FileName[EFS_FILENAME_LEN];

    uint8_t Mode;
    
    uint16_t DataPage;
    uint16_t PageOffset;
    uint16_t DataOffset;
    uint16_t FileSize;

    Efs_FileHdr_t FileHdr;
}Efs_Fd_t;

/* EFS File Mode */
#define EFS_FREAD   (1)
#define EFS_FWRITE  (2)
#define EFS_FRW     (3)

/* EFS Macros */
#define EFS_MAX(x,y)            (((x) > (y)) ? (x) : (y))
#define EFS_MIN(x,y)            (((x) < (y)) ? (x) : (y))
#define EFS_ROUND_UP(x, align)	(((int) (x) + (align - 1)) & ~(align - 1))
#define EFS_CEIL(num,den)       ( ((num)/(den)) + (((num)%(den))!=0) )
#define EFS_NXT_PAGE_ADDR(page) ((page)*Efs_FsHdr.PageSize + EFS_PGDATA_SIZE)

/******************************************************************/
#endif
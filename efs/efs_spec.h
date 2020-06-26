#ifndef __EFS_SPEC_H__
#define __EFS_SPEC_H__

#include <stdint.h>

#include "efs_config.h"

#if defined (__GNUC__)
   #define _EXTENSION_    __extension__
   #define OS_PACK        __attribute__ ((packed))
   #define OS_ALIGN(n)  __attribute__((aligned(n)))
#else
   #define _EXTENSION_ 
   #define OS_PACK
   #define OS_ALIGN(n) 
#endif


/*****************************************************************/
typedef struct OS_PACK EfsFsHdr
{
    uint16_t MagicKey;
    uint16_t Checksum;
    uint16_t PageSize;
    uint16_t PageCount;
    uint16_t MaxFileCount;
    uint16_t BitmapSize;
    uint16_t DataStartPage;
} Efs_FsHdr_t;

#define EFS_FS_MAGIC_KEY    (0x0EF5)
#define EFS_FS_HDR_SIZE     (sizeof(Efs_FsHdr_t))
#define EFS_FS_START_ADDR   (0)
#define EFS_BM_START_ADDR   (EFS_FS_HDR_SIZE)

/*****************************************************************/
#define EFS_FILE_START_ADDR (EFS_FS_HDR_SIZE+Efs_FsHdr.BitmapSize)
#define EFS_PGDATA_SIZE     (Efs_FsHdr.PageSize - 2)
#define EFS_FILENAME_LEN    (11)    

typedef struct OS_PACK EfsFileHdr
{
    uint8_t Attr;
    uint16_t FileSize;
    uint16_t PageAddr;
    char FileName[EFS_FILENAME_LEN];
} Efs_FileHdr_t;

#define EFS_FILE_ATTR_INUSE_MASK   (0x01)
#define EFS_FILE_ATTR_W_MASK       (0x10)
#define EFS_FILE_ATTR_R_MASK       (0x20)
#define EFS_FILE_ATTR_RW_MASK      (0x30)

#define EFS_FILE_HDR_SIZE   (sizeof(Efs_FileHdr_t))

/*****************************************************************/
/*#define EFS_DB_DATA_SIZE    ((EFS_PAGE_SIZE)-(sizeof(uint16_t)))
typedef struct OS_PACK EfsDataBlock
{
    uint8_t Data[EFS_DB_DATA_SIZE];
    uint16_t NextBlock;
}Efs_DataBlock_t;*/

/*****************************************************************/

#endif
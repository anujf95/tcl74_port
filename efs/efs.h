#ifndef __EFS_H__
#define __EFS_H__

#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef __EFS_SPEC_H__
typedef struct __attribute__ ((packed)) EfsFsHdr
{
    uint16_t MagicKey;
    uint16_t Checksum;
    uint16_t PageSize;
    uint16_t PageCount;
    uint16_t MaxFileCount;
    uint16_t BitmapSize;
    uint16_t DataStartPage;
} Efs_FsHdr_t;
#endif

/* EFS Return Codes */
#define EFS__SUCCESS                ( 0) 
#define EFS__ERROR                  (-1)
#define EFS__INVALID_ARGUMENT       (-2)
#define EFS__PERMISSION_DENIED      (-4)
#define EFS__FILE_NOT_FOUND         (-5)
#define EFS__INVALID_FILE_SYSTEM    (-6)
#define EFS__FD_TABLE_FULL          (-7)
#define EFS__FS_FILE_FULL           (-8)
#define EFS__NO_SPACE_LEFT          (-9)


/******************************************************************/
int32_t EfsFsLoad(void);
int32_t EfsFsCreate(Efs_FsHdr_t FsHdr);
int32_t EfsFileSetAttr(char *FileName,uint8_t attr);
int32_t EfsHasOpenFiles(void);
int32_t EfsGetFileGlob(char *FileName, int32_t init);
/******************************************************************/
int32_t efs_creat(char *FileName, uint32_t flags, uint8_t attr);
int32_t efs_open(char *FileName,uint32_t flags);
int32_t efs_close(int32_t fd);
int32_t efs_read(int32_t fd, uint8_t *data, uint32_t size);
int32_t efs_write(int32_t fd, uint8_t *data, uint32_t size);
int32_t eefs_lseek(int32_t fd,off_t offset, int32_t whence);
int32_t efs_fstat(int32_t fd,struct stat *statbuf);
int32_t efs_stat(char *FileName, struct stat *statbuf);
int32_t eefs_remove(char *FileName);
int32_t eefs_rename(char *OldName, char *NewName);
#endif
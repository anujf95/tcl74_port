#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "efs_macro.h"
#include "efs.h"
#include "efs_ll.h"

/****************************************************************/
static Efs_FsHdr_t Efs_FsHdr;
static Efs_Fd_t Efs_FdTable[EFS_MAX_OPEN_FILES];
#if defined(EFS_MALLOC_SUPPORT) && (EFS_MALLOC_SUPPORT)
    static uint8_t *Efs_Bm = NULL;
    static uint8_t *Dbuf = NULL;
#else
    #if !defined(EFS_BM_SIZE)
        #error "EFS_BM_SIZE needs to be defined if EFS_MALLOC_SUPPORT is NOT available"
    #else
        static uint8_t Efs_Bm[EFS_BM_SIZE];
    #endif

    #if !defined(EFS_DBUF_SIZE)
        #error "EFS_DBUF_SIZE needs to be defined if EFS_MALLOC_SUPPORT is NOT available"
    #else
        static uint8_t Dbuf[EFS_DBUF_SIZE];
    #endif
#endif

/****************************************************************/
static int32_t EfsBmRead(uint16_t PageNo)
{
    if(PageNo>=Efs_FsHdr.PageCount)
        return EFS__INVALID_ARGUMENT;

    uint8_t ByteOffset,BitOffset;
    ByteOffset = PageNo/8;
    BitOffset  = PageNo%8;

    if(Efs_Bm[ByteOffset] & (1<<BitOffset))
        return 1;
    else
        return 0;
}

static int32_t EfsBmWrite(uint16_t PageNo,uint8_t value)
{
    if(PageNo>=Efs_FsHdr.PageCount)
        return EFS__INVALID_ARGUMENT;

    uint8_t ByteOffset,BitOffset;
    ByteOffset = PageNo/8;
    BitOffset  = PageNo%8;

    if(value)
        Efs_Bm[ByteOffset] |= (1<<BitOffset);
    else
        Efs_Bm[ByteOffset] &= ~(1<<BitOffset);

    return EFS__SUCCESS;
}

static int32_t EfsBmFlush(void)
{
    if(efs_ll_write(Efs_Bm,EFS_FS_HDR_SIZE,Efs_FsHdr.BitmapSize)<Efs_FsHdr.BitmapSize)
        return EFS__ERROR;
    else
        return EFS__SUCCESS;
}

static int32_t EfsFsFlush(void)
{
    if(efs_ll_write((uint8_t *)&Efs_FsHdr,0,EFS_FS_HDR_SIZE)<EFS_FS_HDR_SIZE)
        return EFS__ERROR;
    else
        return EFS__SUCCESS;
}

static int32_t EfsResolveFilePage(Efs_FileHdr_t *file,uint16_t DataOffset, uint16_t *DataPage, uint16_t *PageOffset)
{
    uint16_t iPage,iIndex;

    //init to file origin
    iPage = file->PageAddr;
    iIndex = 0;

    while((DataOffset-iIndex)>EFS_PGDATA_SIZE)
    {
        if(efs_ll_read((uint8_t *)&iPage,EFS_NXT_PAGE_ADDR(iPage),2)<2)
            return EFS__ERROR;
        
        if( (iPage != 0) && ((DataOffset-iIndex)>EFS_PGDATA_SIZE) ) //Not last Page && expecting More pages
            iIndex += EFS_PGDATA_SIZE;
        else if((iPage == 0) && ((DataOffset-iIndex)>EFS_PGDATA_SIZE)) //Last Page && Not epecting More Pages
            return EFS__INVALID_ARGUMENT;
    }
    
    //Reached the required DataOffset
    *DataPage = iPage;
    *PageOffset = DataOffset-iIndex;
    return DataOffset;
}

static int32_t EfsCleanFileBitmap(Efs_FileHdr_t *file)
{
    uint16_t page = file->PageAddr;
    while(page != 0)
    {
        EfsBmWrite(page,0);
        if(efs_ll_read((uint8_t *)&page,EFS_NXT_PAGE_ADDR(page),2)<2)
            return EFS__ERROR;
    }
    return EfsBmFlush();
}

static int32_t EfsGetFreePage(void)
{
    uint16_t page;
    for(page=Efs_FsHdr.DataStartPage;page<Efs_FsHdr.PageCount;page++)
    {
        if(!EfsBmRead(page))
            return page;
    }
    return EFS__NO_SPACE_LEFT;
}

static int32_t EfsIsValidFileName(char *filename)
{
    if(filename != NULL)
    {
        uint8_t filelen = strlen(filename);
        if( (filelen>0) && (filelen<=EFS_FILENAME_LEN))
            return EFS__SUCCESS;
    }
    return EFS__ERROR;
        
}

static int32_t EfsFindFile(char *filename, uint8_t *FileIndex, Efs_FileHdr_t *FileHdr)
{
    /* Assumes filename is valid & No IO error*/
    uint8_t i;
    uint32_t FileAddr;
    Efs_FileHdr_t file;
    for(i=0; i<Efs_FsHdr.MaxFileCount; i++)
    {
        FileAddr = EFS_FILE_START_ADDR + (i*EFS_FILE_HDR_SIZE);
        efs_ll_read((uint8_t *)&file,FileAddr,EFS_FILE_HDR_SIZE);

        //1. check if file is in use
        if(file.Attr & EFS_FILE_ATTR_INUSE_MASK)
        {
            //2. match for file name
            if(!strcmp(filename,file.FileName))
            {
                if(FileIndex!=NULL)
                    *FileIndex = i;
                if(FileHdr!=NULL)
                    memcpy(FileHdr,&file,EFS_FILE_HDR_SIZE);
                return EFS__SUCCESS;
            }
        }
    }
    return EFS__FILE_NOT_FOUND;
}

static int32_t EfsFileOpen(char *FileName,uint8_t FileIndex,uint32_t flags)
{
    uint8_t FdIndex=0xFF;
    int32_t retval;
    Efs_FileHdr_t file;

    //1. check for implemented flags
    if (flags & ~(O_RDONLY | O_WRONLY | O_RDWR | O_TRUNC | O_CREAT | O_APPEND))
        return EFS__INVALID_ARGUMENT;
    
    //2. Load Header, check for file attrib and flags (permission match)
    efs_ll_read((uint8_t*)&file,EFS_FILE_START_ADDR+FileIndex*EFS_FILE_HDR_SIZE,EFS_FILE_HDR_SIZE);
    if((flags & O_RDONLY) && !(file.Attr & EFS_FILE_ATTR_R_MASK))
        return EFS__PERMISSION_DENIED;
    
    if((flags & O_WRONLY) && !(file.Attr & EFS_FILE_ATTR_W_MASK))
        return EFS__PERMISSION_DENIED;

    //4. check for available slot in fd_table
    for(uint8_t i=0; i<EFS_MAX_OPEN_FILES; i++)
    {
        if(!Efs_FdTable[i].InUse)
        {
            FdIndex = i;
            break;
        }    
    }
    if(FdIndex==0xFF)
        return EFS__FD_TABLE_FULL;

    //5. init entry in fd_table; allocate fd; set file access modes
    strncpy(Efs_FdTable[FdIndex].FileName,FileName,EFS_FILENAME_LEN);
    Efs_FdTable[FdIndex].FileIndex = FileIndex;
    Efs_FdTable[FdIndex].Mode = (flags & O_ACCMODE) + 1;

    if( ((flags&O_ACCMODE)==O_RDONLY) || ((flags&O_ACCMODE)==O_RDWR) ) //"r"
    {
        Efs_FdTable[FdIndex].DataOffset = 0;
        Efs_FdTable[FdIndex].DataPage = file.PageAddr;
        Efs_FdTable[FdIndex].PageOffset = 0;
        Efs_FdTable[FdIndex].FileSize = file.FileSize;
    }
    else if( (((flags&O_ACCMODE)==O_WRONLY)||((flags&O_ACCMODE)==O_RDWR)) && (flags&O_TRUNC) ) //"w"
    {
        Efs_FdTable[FdIndex].DataOffset = 0;
        Efs_FdTable[FdIndex].DataPage = 0;
        Efs_FdTable[FdIndex].PageOffset = 0;
        Efs_FdTable[FdIndex].FileSize = 0;
        EfsCleanFileBitmap(&file);
    }
    else if( (((flags&O_ACCMODE)==O_WRONLY)||((flags&O_ACCMODE)==O_RDWR)) && (flags&O_APPEND) ) //"a"
    {
        Efs_FdTable[FdIndex].DataOffset = file.FileSize;
        if((retval=EfsResolveFilePage(&file,file.FileSize,
            &Efs_FdTable[FdIndex].DataPage,&Efs_FdTable[FdIndex].PageOffset))<0)
            return retval;
        Efs_FdTable[FdIndex].FileSize = file.FileSize;
    }

    memcpy(&Efs_FdTable[FdIndex].FileHdr,&file,EFS_FILE_HDR_SIZE);
    Efs_FdTable[FdIndex].InUse = 1;

    //6. return fd
    return (FdIndex+EFS_FD_OFFSET);
}

static int32_t EfsFileCreat(char *FileName,uint8_t attr)
{
    uint8_t FdIndex=0xFF,FileIndex=0xFF;
    Efs_FileHdr_t file;

    //1. check for available slot in fd_table
    for(uint8_t i=0; i<EFS_MAX_OPEN_FILES; i++)
    {
        if(!Efs_FdTable[i].InUse)
        {
            FdIndex = i;
            break;
        }    
    }
    if(FdIndex==0xFF)
        return EFS__FD_TABLE_FULL;

    //2. make sure enough room to accomodate file_hdr
    for (uint8_t i=0; i<Efs_FsHdr.MaxFileCount; i++)
    {
        efs_ll_read((uint8_t*)&file,(EFS_FILE_START_ADDR+i*EFS_FILE_HDR_SIZE),EFS_FILE_HDR_SIZE);
        if(!(file.Attr & EFS_FILE_ATTR_INUSE_MASK))
        {
            FileIndex = i;
            break;
        }
    }
    if(FileIndex==0xFF)
        return EFS__FS_FILE_FULL;
    
    //3.1 fill attribute, file_name data
    file.Attr = attr | EFS_FILE_ATTR_INUSE_MASK;
    strncpy(file.FileName,FileName,EFS_FILENAME_LEN);
    file.FileSize = 0xFFFF;
    file.PageAddr = 0;

    //3.2 create entry in fd_table and initilise it
    Efs_FdTable[FdIndex].InUse = 1;
    Efs_FdTable[FdIndex].FileIndex = FileIndex;
    strncpy(Efs_FdTable[FdIndex].FileName,FileName,EFS_FILENAME_LEN);
    Efs_FdTable[FdIndex].Mode = EFS_FWRITE;
    Efs_FdTable[FdIndex].DataOffset = 0;
    Efs_FdTable[FdIndex].FileSize = 0;
    memcpy(&Efs_FdTable[FdIndex].FileHdr,&file,EFS_FILE_HDR_SIZE);

    //3.3 flush file_hdr
    efs_ll_write((uint8_t*)&file,EFS_FILE_START_ADDR+FileIndex*EFS_FILE_HDR_SIZE,EFS_FILE_HDR_SIZE);

    //3.4 return fd
    return (FdIndex+EFS_FD_OFFSET);
}
/****************************************************************/
int32_t EfsFsLoad(void)
{
    Efs_FsHdr_t FsHdr;
    if(efs_ll_read((uint8_t *)&FsHdr,0,EFS_FS_HDR_SIZE)<EFS_FS_HDR_SIZE)
        return EFS__ERROR;

    //check 1: EEFS Magic Key check
    if(FsHdr.MagicKey != EFS_FS_MAGIC_KEY)
        return EFS__INVALID_FILE_SYSTEM;
    

    //check 2: invalid Page size and count check
    if( (FsHdr.PageCount == 0)    || 
        (FsHdr.PageSize%16 != 0)  ||
        (FsHdr.BitmapSize == 0))
        return EFS__INVALID_FILE_SYSTEM;

    //everything checked (OK) => Now init fs
    //1. copy header
    memcpy(&Efs_FsHdr,&FsHdr,EFS_FS_HDR_SIZE);

    //2. clear file descriptor table
    for(uint32_t i=0; i<EFS_MAX_OPEN_FILES; i++)
        memset((void *)&Efs_FdTable[i],0,sizeof(Efs_Fd_t));

    //3. allocate memory to bitmap; copy the bitmap
    #if defined(EFS_MALLOC_SUPPORT)
    if(Efs_Bm != NULL)
        free(Efs_Bm);
    Efs_Bm = (uint8_t *)malloc(Efs_FsHdr.BitmapSize);
    efs_ll_read(Efs_Bm,EFS_BM_START_ADDR,Efs_FsHdr.BitmapSize);
    #else
        if(Efs_FsHdr.BitmapSize != EFS_BM_SIZE)
            return EFS__ERROR;
        else
            eefs_ll_read(Efs_Bm,EFS_BM_START_ADDR,EFS_BM_SIZE);
    #endif

    //4. allocate memory to Page Buffer
    #if defined(EFS_MALLOC_SUPPORT)
    if(Dbuf != NULL)
        free(Dbuf);
    Dbuf = (uint8_t *)malloc(Efs_FsHdr.PageSize);
    #else
        if(Efs_FsHdr.PageSize != EFS_DBUF_SIZE)
            return EFS__ERROR;
    #endif
    
    return EFS__SUCCESS;
}

int32_t EfsFsCreate(Efs_FsHdr_t FsHdr)
{
    //Write Fs Header
    if(efs_ll_write((uint8_t *)&FsHdr,0,EFS_FS_HDR_SIZE)<EFS_FS_HDR_SIZE)
        return EFS__ERROR;
        
    //erase all bitmap data
    if(efs_ll_fill(0,EFS_BM_START_ADDR,FsHdr.BitmapSize)<FsHdr.BitmapSize)
        return EFS__ERROR;

    //erase all file header data
    if(efs_ll_fill(0,FsHdr.DataStartPage*FsHdr.PageSize,
        FsHdr.MaxFileCount*EFS_FILE_HDR_SIZE)<(FsHdr.MaxFileCount*EFS_FILE_HDR_SIZE))
        return EFS__ERROR;
    return EFS__SUCCESS; 
}

int32_t EfsFileSetAttr(char *FileName,uint8_t attr)
{
    uint8_t FileIndex;
    int32_t retval;
    Efs_FileHdr_t file;

    //1. check for invalid file name
    if(EfsIsValidFileName(FileName)<EFS__SUCCESS)
        return EFS__INVALID_ARGUMENT;
    
    //2. check if file exists in file system
    if((retval=EfsFindFile(FileName,&FileIndex,&file))<0)
        return retval;

    //3. update file attr
    attr = ((attr&0x6)>>1);
    file.Attr = (file.Attr & ~EFS_FILE_ATTR_RW_MASK) | (attr<<4);

    if(efs_ll_write((uint8_t*)&file,EFS_FILE_START_ADDR+FileIndex*EFS_FILE_HDR_SIZE,
        EFS_FILE_HDR_SIZE)<EFS_FILE_HDR_SIZE)
        return EFS__ERROR;
    
    return EFS__SUCCESS;

}

int32_t EfsHasOpenFiles(void)
{
    int32_t OpenFiles=0;

    for(int8_t i=0; i<EFS_MAX_OPEN_FILES;i++)
    {
        if(Efs_FdTable[i].InUse)
            OpenFiles++;
    }

    return OpenFiles;
}

int32_t EfsGetFileGlob(char *FileName, int32_t init)
{
	static uint8_t FileIndex = 0;
	uint8_t i;
    uint32_t FileAddr;
    Efs_FileHdr_t file;
	
	if(init)
		FileIndex = 0;
	
    for(i=FileIndex; i<Efs_FsHdr.MaxFileCount; i++)
    {
        FileAddr = EFS_FILE_START_ADDR + (i*EFS_FILE_HDR_SIZE);
        efs_ll_read((uint8_t *)&file,FileAddr,EFS_FILE_HDR_SIZE);

        //1. check if file is in use
        if(file.Attr & EFS_FILE_ATTR_INUSE_MASK)
        {
            strcpy(FileName,file.FileName);
			FileIndex +=1;
            return 1;
        }
		else
			continue;
    }
	return 0;
}
/****************************************************************/

int32_t efs_creat(char *FileName, uint32_t flags, uint8_t attr)
{
    uint8_t FileIndex;
    if(EfsIsValidFileName(FileName)<EFS__SUCCESS)
        return EFS__INVALID_ARGUMENT;

    //1. check if file exists in file system
    if(EfsFindFile(FileName,&FileIndex,NULL) != EFS__FILE_NOT_FOUND)
    {
        return EfsFileOpen(FileName,FileIndex,flags);
    }
    //2. check if file needs to be created
    else
    {   
        if(attr==0) attr=EFS_FILE_ATTR_RW_MASK;
        return EfsFileCreat(FileName,attr);
    }
}

int32_t efs_open(char *FileName,uint32_t flags)
{
    uint8_t FileIndex;
    if(EfsIsValidFileName(FileName)<EFS__SUCCESS)
        return EFS__INVALID_ARGUMENT;
    
    //1. check if file exists in file system
    if(EfsFindFile(FileName,&FileIndex,NULL) != EFS__FILE_NOT_FOUND)
    {
        return EfsFileOpen(FileName,FileIndex,flags);
    }
    //2. check if file needs to be created
    else if(flags & O_CREAT)
    {
        return EfsFileCreat(FileName,EFS_FILE_ATTR_RW_MASK);
    }
    else
        return EFS__FILE_NOT_FOUND;
}

int32_t efs_close(int32_t fd)
{
    
    Efs_Fd_t *Fd = &Efs_FdTable[fd-EFS_FD_OFFSET];

    //1. Check for valid fd
    if(Fd->InUse ==0)
        return EFS__INVALID_ARGUMENT;
    
    //2. Check for File Opening type
    if(Fd->Mode & EFS_FWRITE)
    {
        //Mark Current Page as Last Page
        if(efs_ll_fill(0,(Fd->DataPage*Efs_FsHdr.PageSize)+EFS_PGDATA_SIZE,2)<2)
            return EFS__ERROR;
        
        //update file header
        Fd->FileHdr.FileSize = Fd->FileSize;
        if(efs_ll_write((uint8_t*)&Fd->FileHdr,EFS_FILE_START_ADDR+(Fd->FileIndex*EFS_FILE_HDR_SIZE),
            EFS_FILE_HDR_SIZE)<EFS_FILE_HDR_SIZE)
            return EFS__ERROR;
    }

    //3. clean the fd_table entry
    memset(Fd,0,sizeof(Efs_Fd_t));
    return EFS__SUCCESS;
}

int32_t efs_read(int32_t fd, uint8_t *data, uint32_t size)
{
    int16_t cnt=0,NextPage;
    Efs_Fd_t *Fd = &Efs_FdTable[fd-EFS_FD_OFFSET];

    //1. Check for valid fd
    if(Fd->InUse ==0)
        return EFS__INVALID_ARGUMENT;
    
    //2. Validate data
    if(data==NULL)
        return EFS__INVALID_ARGUMENT;
    
    //3. Verify File Mode
    if(!(Fd->Mode&EFS_FREAD))
        return EFS__PERMISSION_DENIED;
    
    //4. Return 0 if EOF reached
    if(Fd->DataOffset>=Fd->FileSize)
        return 0;
    
    //5. read data
    while(cnt<=EFS_MIN(size,Fd->FileSize))
    {
        if((EFS_MIN(size,Fd->FileSize)-cnt)>(EFS_PGDATA_SIZE - Fd->PageOffset))
        {
            //data to be read will require more pages
            if(efs_ll_read(&data[cnt],(Fd->DataPage*Efs_FsHdr.PageSize)+Fd->PageOffset,
                (EFS_PGDATA_SIZE - Fd->PageOffset))<(EFS_PGDATA_SIZE - Fd->PageOffset))
                return EFS__ERROR;
            
            cnt += (EFS_PGDATA_SIZE - Fd->PageOffset);
            Fd->PageOffset = 0;
            if(efs_ll_read((uint8_t *)&NextPage,(Fd->DataPage*Efs_FsHdr.PageSize)+EFS_PGDATA_SIZE,2)<2)
                return EFS__ERROR;
            if(NextPage==0)
                return cnt; //should not happen
            else
                Fd->DataPage = NextPage;
            
        }
        else
        {
            if(efs_ll_read(&data[cnt],(Fd->DataPage*Efs_FsHdr.PageSize)+Fd->PageOffset,
                EFS_MIN(size,Fd->FileSize)-cnt)<(EFS_MIN(size,Fd->FileSize)-cnt))
                return EFS__ERROR;
            
            Fd->PageOffset += (EFS_MIN(size,Fd->FileSize)-cnt);
            cnt = EFS_MIN(size,Fd->FileSize);
            break;
        }
    }
    Fd->DataOffset += cnt;
    return cnt;
}

int32_t efs_write(int32_t fd, uint8_t *data, uint32_t size)
{
    int16_t cnt=0,NextPage;
    Efs_Fd_t *Fd = &Efs_FdTable[fd-EFS_FD_OFFSET];

    //1. Check for valid fd
    if(Fd->InUse ==0)
        return EFS__INVALID_ARGUMENT;
    
    //2. Validate data
    if(data==NULL)
        return EFS__INVALID_ARGUMENT;
    
    //3. Verify File Mode
    if(!(Fd->Mode&EFS_FWRITE))
        return EFS__PERMISSION_DENIED;
    
    //4. check if new page to be allocated
    if(Fd->DataPage==0)
    {
        Fd->DataPage = EfsGetFreePage();
        Fd->FileHdr.PageAddr = Fd->DataPage;
    }
    if(Fd->DataPage==EFS__NO_SPACE_LEFT)
        return EFS__NO_SPACE_LEFT;

    //5. write data
    while(cnt<=size)
    {
        if((size-cnt)>(EFS_PGDATA_SIZE - Fd->PageOffset))
        {
            //data to be written will require more pages
            if(efs_ll_write(&data[cnt],(Fd->DataPage*Efs_FsHdr.PageSize)+Fd->PageOffset,
                (EFS_PGDATA_SIZE - Fd->PageOffset))<(EFS_PGDATA_SIZE - Fd->PageOffset))
                return EFS__ERROR;
            
            cnt += (EFS_PGDATA_SIZE - Fd->PageOffset);
            Fd->PageOffset = 0;
            EfsBmWrite(Fd->DataPage,1);
            NextPage = EfsGetFreePage();
            if(efs_ll_write((uint8_t *)&NextPage,(Fd->DataPage*Efs_FsHdr.PageSize)+EFS_PGDATA_SIZE,2)<2)
                return EFS__ERROR;
            if(NextPage==EFS__NO_SPACE_LEFT)
            {
                //TODO: File System Full Wrap Up handling
            }
            else
                Fd->DataPage = NextPage;
            
        }
        else
        {
            if(efs_ll_write(&data[cnt],(Fd->DataPage*Efs_FsHdr.PageSize)+Fd->PageOffset,
                size-cnt)<(size-cnt))
                return EFS__ERROR;
            
            Fd->PageOffset += (size-cnt);
            cnt = size;
            EfsBmWrite(Fd->DataPage,1);
            break;
        }
    }
    //6. Update File Size
    Fd->FileSize += cnt;

    //7. flush Bitmap Table
    if(EfsBmFlush()!=EFS__SUCCESS)
        return EFS__ERROR;
    
	Fd->DataOffset += cnt;
    return cnt;
}

int32_t eefs_lseek(int32_t fd,off_t offset, int32_t whence)
{
    int32_t retval;
    int16_t DataOffset;
    Efs_Fd_t *Fd = &Efs_FdTable[fd-EFS_FD_OFFSET];

    //1. check for valid fd
    if(Fd->InUse ==0)
        return EFS__INVALID_ARGUMENT;

    if(whence==SEEK_SET)
        DataOffset = offset;
    else if(whence==SEEK_CUR)
        DataOffset = Fd->DataOffset + offset;
    else if(whence==SEEK_END)
        DataOffset = Fd->FileSize;
    else
        return EFS__INVALID_ARGUMENT;

    if(DataOffset>Fd->FileSize)
        return EFS__ERROR;
    else if(DataOffset<0)
        return EFS__ERROR;

    retval = EfsResolveFilePage(&Fd->FileHdr,DataOffset,&Fd->DataPage,&Fd->PageOffset);
    if(retval<0)
        return retval;
    else
        return EFS__SUCCESS;
}

int32_t efs_fstat(int32_t fd,struct stat *statbuf)
{
    Efs_Fd_t *Fd = &Efs_FdTable[fd-EFS_FD_OFFSET];

    //1. check for valid fd
    if(Fd->InUse ==0)
        return EFS__INVALID_ARGUMENT;
    
    if(statbuf==NULL)
        return EFS__INVALID_ARGUMENT;

    statbuf->st_ino = Fd->FileIndex;    //Inode number = file Index
    statbuf->st_uid = 0;            //UID of file = root
    statbuf->st_gid = 0;            //GID of file = root
    statbuf->st_size = Fd->FileSize;
    statbuf->st_blksize = Efs_FsHdr.PageSize;
    statbuf->st_mode = _IFREG;
    if((Fd->FileHdr.Attr&EFS_FILE_ATTR_R_MASK))
        statbuf->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH);
    if((Fd->FileHdr.Attr&EFS_FILE_ATTR_W_MASK))
        statbuf->st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);

    return EFS__SUCCESS;
}

int32_t efs_stat(char *FileName, struct stat *statbuf)
{
    uint8_t FileIndex;
    int32_t retval;
    Efs_FileHdr_t file;
	
	
    //1. check for invalid file name
    if(EfsIsValidFileName(FileName)<EFS__SUCCESS)
        return EFS__INVALID_ARGUMENT;
    
    if(statbuf==NULL)
        return EFS__INVALID_ARGUMENT;

	//0. Directory check
	if(!strcmp(FileName,".") || !strcmp(FileName,"/"))
	{
		statbuf->st_mode = _IFDIR;
        statbuf->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH);
        statbuf->st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
		return EFS__SUCCESS;
	}
		
    //2. check if file exists in file system
    if((retval=EfsFindFile(FileName,&FileIndex,&file))<0)
        return retval;

    statbuf->st_ino = FileIndex;    //Inode number = file Index
    statbuf->st_uid = 0;            //UID of file = root
    statbuf->st_gid = 0;            //GID of file = root
    statbuf->st_size = file.FileSize;
    statbuf->st_blksize = Efs_FsHdr.PageSize;
    statbuf->st_mode = _IFREG;
    if((file.Attr&EFS_FILE_ATTR_R_MASK))
        statbuf->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH);
    if((file.Attr&EFS_FILE_ATTR_W_MASK))
        statbuf->st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);

    return EFS__SUCCESS;
}

int32_t eefs_remove(char *FileName)
{
    uint8_t FileIndex;
    int32_t retval;
    Efs_FileHdr_t file;
    //1. check for invalid file name
    if(EfsIsValidFileName(FileName)<EFS__SUCCESS)
        return EFS__INVALID_ARGUMENT;
    
    //2. check if file exists in file system
    if((retval=EfsFindFile(FileName,&FileIndex,&file))<0)
        return retval;

    //3. clean file data bitmap
    if((retval=EfsCleanFileBitmap(&file))<0)
        return retval;
    
    //4. clean file header
    if(efs_ll_fill(0,EFS_FILE_START_ADDR+FileIndex*EFS_FILE_HDR_SIZE,EFS_FILE_HDR_SIZE)<EFS_FILE_HDR_SIZE)
        return EFS__ERROR;
    
    return EFS__SUCCESS;
}

int32_t eefs_rename(char *OldName, char *NewName)
{
    uint8_t FileIndex;
    int32_t retval;
    Efs_FileHdr_t file;

    //1. check for invalid file name
    if(EfsIsValidFileName(OldName)<EFS__SUCCESS)
        return EFS__INVALID_ARGUMENT;

    if(EfsIsValidFileName(NewName)<EFS__SUCCESS)
        return EFS__INVALID_ARGUMENT;
    
    //2. check if file exists in file system
    if((retval=EfsFindFile(OldName,&FileIndex,&file))<0)
        return retval;
    
    //3. Update File Name
    strncpy(file.FileName,NewName,EFS_FILENAME_LEN);

    if(efs_ll_write((uint8_t*)&file,EFS_FILE_START_ADDR+FileIndex*EFS_FILE_HDR_SIZE,
        EFS_FILE_HDR_SIZE)<EFS_FILE_HDR_SIZE)
        return EFS__ERROR;
    
    return EFS__SUCCESS;
}
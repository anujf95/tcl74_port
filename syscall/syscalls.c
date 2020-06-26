/*
 * syscall_retarget.c
 *
 *  Created on: 13-Mar-2020
 *      Author: anuj
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <reent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "syscalls.h"
#include "uart_stream.h"
#include "efs.h"

/*************************************SYSCALLS********************************************/
#undef errno
extern int errno;


int _close(int fd)
{
	int32_t retval;
	if(fd<=STDERR_FILENO) {
		errno = EBADF;
        return -1;
	}
	else
	{
		switch(efs_close(fd))
		{
			case EFS__INVALID_ARGUMENT:
				errno = EINVAL;
				return -1;
			case EFS__ERROR:
				errno = EIO;
				return -1;
				break;
			case EFS__SUCCESS:
				return 0;
			break;
			default:
				return -1;
		}
    }
}

int _execve(const char *name, char *const argv[], char *const env[])
{
    errno = ENOMEM;
    return -1;
}

int _fcntl(int fd, int flag, int arg)
{
    errno = ENOSYS;
    return -1;
}

int _fork(void)
{
    errno = EAGAIN;
    return -1;
}

int _fstat(int fd, struct stat *pstat)
{
	
	if(fd<=STDERR_FILENO)
	{
        pstat->st_mode = S_IFCHR;
        return 0;
	}
	else
    {
		switch(efs_fstat(fd,pstat))
		{
			case EFS__INVALID_ARGUMENT:
				errno = EBADF;
				return -1;
			case EFS__SUCCESS:
				return 0;
			default:
				errno = EIO;
				return -1;
		}
    }
}

int _getpid(void)
{
    return 1;
}

int _gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
    //TODO:
	errno = EFAULT;
	return -1;
}

int _isatty(int fd)
{
    switch (fd)
    {
    case STDIN_FILENO:
    case STDOUT_FILENO:
    case STDERR_FILENO:
        return 1;
    default:
        errno = EBADF;
        return -1;
    }
}


int _kill(int pid, int sig)
{
    errno = EPERM;
    return -1;
}

int _link(const char *old, const char *new)
{
	switch(eefs_rename(old,new))
	{
		case EFS__INVALID_ARGUMENT:
			errno = ENOENT;
			return -1;
		case EFS__ERROR:
			errno = EIO;
			return -1;
		case EFS__SUCCESS:
			return 0;
		default:
			errno = EPERM;
			return -1;
	}
}

off_t _lseek(int fd, off_t pos, int whence)
{
    if(fd<=STDERR_FILENO)
        return 0;
    else
	{
		switch(eefs_lseek(fd,pos,whence))
		{
			case EFS__INVALID_ARGUMENT:
				errno = EBADF;
				return -1;
			case EFS__ERROR:
				errno = EIO;
				return -1;
			case EFS__SUCCESS:
				return 0; //TODO: verify
			default:
				errno = EBADF;
				return -1;
		} 
    }
}

int _open(const char *file, int flags, int mode)
{
	int retval = efs_open(file,flags);
	if(retval>0)
		return retval;
	
	switch(retval)
	{
		case EFS__INVALID_ARGUMENT:
			errno = ENOENT;
			return -1;
		case EFS__FILE_NOT_FOUND:
			errno = ENOENT;
			return -1;
		case EFS__ERROR:
			errno = EIO;
			return -1;
		case EFS__FD_TABLE_FULL:
			errno = ENOMEM;
			return -1;
		case EFS__FS_FILE_FULL:
			errno = ENOMEM;
			return -1;
		default:
			errno = EACCES;
			return -1;
	}
}

int _read(int fd, void *buf, size_t cnt)
{
	int retval;
    switch (fd)
    {
    case STDIN_FILENO:
        return uart_stream_read((char *)buf, cnt);

    case STDOUT_FILENO:
    case STDERR_FILENO:
        errno = EBADF;
        return -1;

    default:
		retval = efs_read(fd,buf,cnt);
		if(retval>=0)
			return retval;
		else
		{
			switch(retval)
			{
				case EFS__INVALID_ARGUMENT:
					errno = EBADF;
					return -1;
				case EFS__ERROR:
					errno = EIO;
					return -1;
				case EFS__PERMISSION_DENIED:
					errno = EACCES;
					return -1;
				default:
					errno = EBADF;
					return -1;
			}
		}
    }
}

/*void *_sbrk(ptrdiff_t incr)
{
    //char *prev_heap_end;

    if (heap_end == 0)
    {
        heap_end = &_end;
    }
    prev_heap_end = heap_end;

    if (heap_end + incr > get_stack_top())
    {
        xprintf("Heap and stack collision\n");
        abort();
    }
#endif
    heap_end += incr;
    return (caddr_t)prev_heap_end;
    return NULL;
}
*/
int _stat(const char *file, struct stat *pstat)
{
	switch(efs_stat(file,pstat))
	{
		case EFS__INVALID_ARGUMENT:
			errno = ENOENT;
			return -1;
		case EFS__FILE_NOT_FOUND:
			errno = ENOENT;
			return -1;
		case EFS__ERROR:
			errno = EIO;
			return -1;
		case EFS__FD_TABLE_FULL:
			errno = ENOMEM;
			return -1;
		case EFS__FS_FILE_FULL:
			errno = ENOMEM;
			return -1;
		case EFS__SUCCESS:
			return 0;
		default:
			errno = EACCES;
			return -1;
	}
}

clock_t _times(struct tms *buf)
{
    errno = EACCES;
    return -1;
}

int _unlink(const char *name)
{
	int retval = eefs_remove(name);
	switch(retval)
	{
		case EFS__INVALID_ARGUMENT:
			errno = ENOENT;
			return -1;
		case EFS__FILE_NOT_FOUND:
			errno = ENOENT;
			return -1;
		case EFS__ERROR:
			errno = EIO;
			return -1;
		case EFS__FD_TABLE_FULL:
			errno = ENOMEM;
			return -1;
		case EFS__FS_FILE_FULL:
			errno = ENOMEM;
			return -1;
		case EFS__SUCCESS:
			return 0;
		default:
			errno = EACCES;
			return -1;
	}
}

pid_t _wait(int *status)
{
    errno = ECHILD;
    return -1;
}

int _write(int fd, const void *buf, size_t cnt)
{
	int retval;
    switch (fd)
    {
    case STDIN_FILENO:
        return 0;

    case STDOUT_FILENO:
    case STDERR_FILENO:
        return uart_stream_write((char *)buf, cnt);
	
    default:
		retval = efs_write(fd,buf,cnt);
		if(retval>=0)
			return retval;
		else
		{
			switch(retval)
			{
				case EFS__INVALID_ARGUMENT:
					errno = ENOENT;
					return -1;
				case EFS__FILE_NOT_FOUND:
					errno = ENOENT;
					return -1;
				case EFS__ERROR:
					errno = EIO;
					return -1;
				case EFS__FD_TABLE_FULL:
					errno = ENOMEM;
					return -1;
				case EFS__FS_FILE_FULL:
					errno = ENOMEM;
					return -1;
				default:
					errno = EACCES;
					return -1;
			}
		}			
   }
}

////////Misc Syscalls
void _exit(int status)
{
    uart_stream_write("exit called with incr", 11);
    while (1)
    {
        ;
    }
}

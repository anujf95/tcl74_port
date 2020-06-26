/*
 * posix_syscalls.c
 *
 *  Created on: 05-May-2020
 *      Author: anuj
 */

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include "dirent2.h"

//XXX: environment setup
static char *__env[] = { "TCL_LIBRARY=", "HOME=/" , NULL};
char **environ = __env;


//change current directory
//Note: returns -1 (error) with errno=EACCES
int chdir(const char *__path )
{
    errno = EACCES;
    return -1;
}

//close a directory
//Note: returns (No error)
void closedir(DIR *dirp)
{
    return;
}

//duplicate a file descriptor
//Note:  returns -1 (error) with EBADF
int dup2(int __fildes, int __fildes2 )
{
    errno = EBADF;
    return -1;
}

//close password entry database
//Note: return
void endpwent (void)
{
    return;
}

//ececutes a file
//Note: Returns -1 with errno=EACCES
int execvp(const char *__file, char * const __argv[] )
{
    errno = EACCES;
    return -1;
}

//Get Current Working Directory
//Note: Currently Returns File System Root
char* getcwd(char *__buf, size_t __size )
{
    strcpy(__buf,"/");
	return __buf;
}

//Get User Identity
//Note: Currently Returns 0 (root)
uid_t geteuid(void )
{
    return 0;
}

//Return user details
//Note: currently returns username "root" with uid=0, gid=0;
struct passwd *getpwnam (const char *name)
{
    static const struct passwd default_user = {
                                           .pw_name = "root",
                                           .pw_uid = 0,
                                           .pw_gid = 0,
                                           .pw_dir = ""
                                        };

    return (struct passwd *) &default_user;
}

static uint8_t dir_init = 1;
//Opens Directory
//Note: Currently returns NULL with errno=EACCES (Permission Denied)
DIR* opendir(char *name)
{
	//xxx: wrapped to handle EFS (which do no support derctories)
	dir_init=1;
	return (void*) 0x0EF5;
    //errno = EACCES;
    //return NULL;
}

//read directory
//Note: Currently return all files in file system (since EFS is flat)
struct dirent* readdir(DIR *dirp)
{
	static struct dirent dir_entry;
	if((uint32_t)dirp == 0x0EF5)
	{
		if(EfsGetFileGlob(dir_entry.d_name, dir_init))
		{
			if(dir_init)
				dir_init=0;
			return &dir_entry;
		}
		else
		{
			if(dir_init)
				dir_init=0;
			return NULL;
		}
	}
	errno = EACCES;
    return NULL;
}

//Creates a Pipe
//Note: Currently return -1 (error) with EFAULT
int pipe(int __fildes[2])
{
    errno = EFAULT;
    return -1;
}

//read value of symbolic link
//Note: Currently return -1 (error) with EACCES
ssize_t readlink(const char *__restrict __path, char *__restrict __buf, size_t __buflen)
{
    errno = EACCES;
    return -1;
}

//create a child process and block parent
//Note: Currently Return -1 as failed
pid_t vfork(void )
{
    return -1;
}

//wait for process to change state
//Note: return -1 with erro=EINVAL (invalid argument)
pid_t waitpid(pid_t pid, int *stat_loc, int options)
{
    errno = EINVAL;
    return -1;
}

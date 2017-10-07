/* This file use for NCTU OSDI course */


// It's handel the file system APIs 
#include <inc/stdio.h>
#include <inc/syscall.h>
#include <fs.h>

/*TODO: Lab7, file I/O system call interface.*/
/*Note: Here you need handle the file system call from user.
 *       1. When user open a new file, you can use the fd_new() to alloc a file object(struct fs_fd)
 *       2. When user R/W or seek the file, use the fd_get() to get file object.
 *       3. After get file object call file_* functions into VFS level
 *       4. Update the file objet's position or size when user R/W or seek the file.(You can find the useful marco in ff.h)
 *       5. Remember to use fd_put() to put file object back after user R/W, seek or close the file.
 *       6. Handle the error code, for example, if user call open() but no fd slot can be use, sys_open should return -STATUS_ENOSPC.
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║   sys_open   ║  file I/O system call interface
 *        ╚══════════════╝
 *               ↓
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   fat_open   │  fat level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    f_open    │  FAT File System Module
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    diskio    │  low level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │     disk     │  simple ATA disk dirver
 *        └──────────────┘
 */
extern struct fs_fd fd_table[FS_FD_MAX];
// Below is POSIX like I/O system call 
int sys_open(const char *file, int flags, int mode)
{
    //We dont care the mode.
/* TODO */
	int fd=-1;
	int i=0;
	for(i;i<FS_FD_MAX;i++)
	{
		if(strcmp (file,fd_table[i].path)==0 && !(flags & O_TRUNC) && !(flags & O_CREAT))
		{
			fd = i;
			break;
		}
	}
	if(fd == -1)
		fd = fd_new();
	else
		fd_get(fd);
	if(fd==-1)
		return -1;
	int ret = file_open(&fd_table[fd], file, flags);
	if(ret <0)
	{
        sys_close(fd);
        return ret;
	}
	// fd_table[fd].size = ((FIL*)fd_table[fd].data)->obj.objsize;
	return fd;
}

int sys_close(int fd)
{
	/* TODO */
	if(fd >= FS_FD_MAX || fd<0)
		return -STATUS_EINVAL;

	int ret = 0;
	if(fd_table[fd].ref_count == 1)
	{
		fd_table[fd].size = 0;
		fd_table[fd].pos = 0;
		memset(fd_table[fd].path, 0, sizeof(fd_table[fd].path));
		ret = file_close(&fd_table[fd]);
	}
	fd_put(&fd_table[fd]);
	return ret;

}
int sys_read(int fd, void *buf, size_t len)
{
/* TODO */
	if(len < 0 || buf == NULL)
		return -STATUS_EINVAL;
	else if (fd >= FS_FD_MAX || fd<0)
		return -STATUS_EBADF;

	int temp=len;
    if(len > (fd_table[fd].size - fd_table[fd].pos))
        temp = fd_table[fd].size - fd_table[fd].pos;

    return file_read(&fd_table[fd], buf, temp);
}
int sys_write(int fd, const void *buf, size_t len)
{
/* TODO */
	if(len < 0 || buf == NULL)
		return -STATUS_EINVAL;
	else if (fd >= FS_FD_MAX || fd<0)
		return -STATUS_EBADF;
    // fd_table[fd].size = ((FIL*)fd_table[fd].data)->obj.objsize;
    return file_write(&fd_table[fd], buf, len);
}

/* Note: Check the whence parameter and calcuate the new offset value before do file_seek() */
off_t sys_lseek(int fd, off_t offset, int whence)
{
/* TODO */
    if(fd >= FS_FD_MAX || fd<0 || offset < 0 || whence < 0)
        return -STATUS_EINVAL;
	int new_offset = 0;
	if(whence == SEEK_SET)
		new_offset = offset;
	else if(whence == SEEK_CUR)
		new_offset = fd_table[fd].pos + offset;
	else if(whence == SEEK_END)
		new_offset = fd_table[fd].size + offset;
	fd_table[fd].pos = new_offset;
	return file_lseek(&fd_table[fd], new_offset);
}

int sys_unlink(const char *pathname)
{
/* TODO */
	return file_unlink(pathname);
}

int sys_opendir(DIR *dir, const char *pathname)
{
	return file_opendir(dir, pathname);
}

int sys_readdir(DIR *dir, FILINFO *file)
{
	return file_readdir(dir, file);
}

int sys_closedir(DIR *dir)
{
	return file_closedir(dir);
}


int sys_mkdir(const char *pathname)
{
	return file_mkdir(pathname);
}
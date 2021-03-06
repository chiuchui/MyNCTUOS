/* This file use for NCTU OSDI course */
/* It's contants fat file system operators */

#include <inc/stdio.h>
#include <fs.h>
#include <fat/ff.h>
#include <diskio.h>

extern struct fs_dev fat_fs;

/*TODO: Lab7, fat level file operator.
 *       Implement below functions to support basic file system operators by using the elmfat's API(f_xxx).
 *       Reference: http://elm-chan.org/fsw/ff/00index_e.html (or under doc directory (doc/00index_e.html))
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   sys_open   │  file I/O system call interface
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║   fat_open   ║  fat level file operator
 *        ╚══════════════╝
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

/* Note: 1. Get FATFS object from fs->data
*        2. Check fs->path parameter then call f_mount.
*/
int fat_mount(struct fs_dev *fs, const void* data)
{
	// FRESULT f_mount (
	// FATFS*       fs,    /* [IN] File system object */
	// const TCHAR* path,  /* [IN] Logical drive number */
	// BYTE         opt    /* [IN] Initialization option */);
	if(fs->path)
		return -f_mount(fs->data, fs->path, 1);

}

/* Note: Just call f_mkfs at root path '/' */
int fat_mkfs(const char* device_name)
{
	// FRESULT f_mkfs (
	// const TCHAR* path,  /* [IN] Logical drive number */
	// BYTE  sfd,          /* [IN] Partitioning rule */
	// UINT  au            /* [IN] Size of the allocation unit */);
	f_mkfs('/', 0, 0);
}

/* Note: Convert the POSIX's open flag to elmfat's flag.
*        Example: if file->flags == O_RDONLY then open_mode = FA_READ
*                 if file->flags & O_APPEND then f_seek the file to end after f_open
*/
int fat_open(struct fs_fd* file)
{
	// FRESULT f_open (
	// FIL* fp,           /* [OUT] Pointer to the file object structure */
	// const TCHAR* path, /* [IN] File name */
	// BYTE mode          /* [IN] Mode flags */);
	// #define O_RDONLY		0x0000000
	// #define O_WRONLY		0x0000001
	// #define O_RDWR			0x0000002
	// #define O_ACCMODE		0x0000003
	// #define O_CREAT			0x0000100
	// #define O_EXCL			0x0000200
	// #define O_TRUNC			0x0001000
	// #define O_APPEND			0x0002000
	// #define O_DIRECTORY		0x0200000
	int flag=0;
	if(file->flags == O_RDONLY)
		flag = FA_READ;
	else if (file-> flags & O_WRONLY)
		flag = FA_WRITE;
	if (file-> flags & O_RDWR)
		flag |= (FA_WRITE | FA_READ);
	if (file-> flags & O_ACCMODE)
		flag &= 0x3;
	if (file-> flags & O_TRUNC)
		flag |= FA_CREATE_ALWAYS;
	else if (file-> flags & O_CREAT)
		flag |= FA_CREATE_NEW;

	int ret = f_open(file->data, file->path, flag);
	file->size = ((FIL*)file->data)->obj.objsize;

	if (file-> flags & O_APPEND)
		f_lseek(file->data, file->size);
	return -ret;
}

int fat_close(struct fs_fd* file)
{
	return -f_close(file->data);
}
int fat_read(struct fs_fd* file, void* buf, size_t count)
{
	// FRESULT f_read (
	// FIL* fp,     /* [IN] File object */
	// void* buff,  /* [OUT] Buffer to store read data */ 
	// UINT btr,    /* [IN] Number of bytes to read */
	// UINT* br     /* [OUT] Number of bytes read */
	unsigned int size;
	int ret = f_read(file->data, buf, count, &size);
	if(ret)
		return -ret;
	file->pos+=size;
	return size;
}
int fat_write(struct fs_fd* file, const void* buf, size_t count)
{
	// FRESULT f_write (
	// FIL* fp,          /* [IN] Pointer to the file object structure */
	// const void* buff,  [IN] Pointer to the data to be written 
	// UINT btw,         /* [IN] Number of bytes to write */
	// UINT* bw          /* [OUT] Pointer to the variable to return number of bytes written */
	unsigned int size;
	int ret = f_write(file->data, buf, count, &size);
	if(ret)
		return -ret;
	file->pos+=size;
	if(file->pos > file->size)
		file->size = file->pos;
	return size;
}
int fat_lseek(struct fs_fd* file, off_t offset)
{
	return -f_lseek(file->data, offset);
}
int fat_unlink(const char *pathname)
{
	return -f_unlink(pathname);
}

int fat_opendir(DIR *dir, const char *pathname)
{
	return -f_opendir(dir, pathname);
}

int fat_readdir(DIR *dir, FILINFO *file)
{
	return -f_readdir(dir, file);
}

int fat_closedir(DIR *dir)
{
	return -f_closedir(dir);
}

int fat_mkdir(const char *pathname)
{
	return -f_mkdir(pathname);
}
struct fs_ops elmfat_ops = {
    .dev_name = "elmfat",
    .mount = fat_mount,
    .mkfs = fat_mkfs,
    .open = fat_open,
    .close = fat_close,
    .read = fat_read,
    .write = fat_write,
    .lseek = fat_lseek,
    .unlink = fat_unlink,
    .opendir = fat_opendir,
    .readdir = fat_readdir,
    .closedir = fat_closedir,
    .mkdir = fat_mkdir,
};




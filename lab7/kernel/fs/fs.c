/* This file use for NCTU OSDI course */
/* It's contants file operator's wapper API */
#include <fs.h>
#include <fat/ff.h>
#include <inc/string.h>
#include <inc/stdio.h>

/* Static file objects */
FIL file_objs[FS_FD_MAX];

/* Static file system object */
FATFS fat;

/* It file object table */
struct fs_fd fd_table[FS_FD_MAX];

/* File system operator, define in fs_ops.c */
extern struct fs_ops elmfat_ops; //We use only one file system...

/* File system object, it record the operator and file system object(FATFS) */
struct fs_dev fat_fs = {
    .dev_id = 1, //In this lab we only use second IDE disk
    .path = {0}, // Not yet mount to any path
    .ops = &elmfat_ops,
    .data = &fat
};
    
/*TODO: Lab7, VFS level file API.
 *  This is a virtualize layer. Please use the function pointer
 *  under struct fs_ops to call next level functions.
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
 *        ╔══════════════╗
 *   ==>  ║  file_open   ║  VFS level file API
 *        ╚══════════════╝
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
// #define STATUS_OK           0       /* no error */
// #define STATUS_ENOENT       2       /* No such file or directory */
// #define STATUS_EIO          5       /* I/O error */
// #define STATUS_ENXIO        6       /* No such device or address */
// #define STATUS_EBADF        9       /* Bad file number */
// #define STATUS_EAGIAN       11      /* Try again */
// #define STATUS_ENOMEM       12      /* no memory */
// #define STATUS_EBUSY        16      /* Device or resource busy */
// #define STATUS_EEXIST       17      /* File exists */
// #define STATUS_EXDEV        18      /* Cross-device link */
// #define STATUS_ENODEV       19      /* No such device */
// #define STATUS_ENOTDIR      20      /* Not a directory */
// #define STATUS_EISDIR       21      /* Is a directory */
// #define STATUS_EINVAL       22      /* Invalid argument */
// #define STATUS_ENOSPC       28      /* No space left on device */
// #define STATUS_EROFS        30      /* Read-only file system */
// #define STATUS_ENOSYS       38      /* Function not implemented */
// #define STATUS_ENOTEMPTY    39      /* Directory not empty */
int error_handle(int error)
{
    int ret = 0;
    switch(error)
    {
        case FR_OK:
            ret = STATUS_OK;
            break;
        case FR_NO_FILE:
        case FR_NO_PATH:
            ret = -STATUS_ENOENT;
            break;
        case FR_DISK_ERR:
        case FR_DENIED:
            ret = -STATUS_EIO;
            break;
        case FR_INVALID_OBJECT:
            ret = -STATUS_ENXIO;
            break;
        case FR_INT_ERR:
        case FR_INVALID_DRIVE:
            ret = -STATUS_EBADF;
            break;
            // ret = STATUS_EAGIAN;
            // break;
            // ret = STATUS_ENOMEM;
            // break;
        case FR_NOT_READY:
        case FR_TIMEOUT:
        case FR_LOCKED:
        case FR_TOO_MANY_OPEN_FILES:
            ret = -STATUS_EBUSY;
            break;
        case FR_EXIST:
            ret = -STATUS_EEXIST;
            break;
            // ret = STATUS_EXDEV;
            // break;
        case FR_NO_FILESYSTEM:
            ret = -STATUS_ENODEV;
            break;
            // ret = STATUS_ENOTDIR;
            // break;
            // ret = STATUS_EISDIR;
            // break;
        case FR_INVALID_NAME:
        case FR_INVALID_PARAMETER:
            ret = -STATUS_EINVAL;
            break;
        case FR_NOT_ENABLED:
            ret = -STATUS_ENOSPC;
            break;
        case FR_WRITE_PROTECTED:
            ret = -STATUS_EROFS;
            break;
        case FR_MKFS_ABORTED:
        case FR_NOT_ENOUGH_CORE:
            ret = -STATUS_ENOSYS;
            break;
            // ret = STATUS_ENOTEMPTY;
            // break;
    }
    return ret;
}
int fs_init()
{
    int res, i;
    
    /* Initial fd_tables */
    for (i = 0; i < FS_FD_MAX; i++)
    {
        fd_table[i].flags = 0;
        fd_table[i].size = 0;
        fd_table[i].pos = 0;
        fd_table[i].type = 0;
        fd_table[i].ref_count = 0;
        fd_table[i].data = &file_objs[i];
        fd_table[i].fs = &fat_fs;
    }
    
    /* Mount fat file system at "/" */
    /* Check need mkfs or not */
    if ((res = fs_mount("elmfat", "/", NULL)) != 0)
    {
        fat_fs.ops->mkfs("elmfat");
        res = fs_mount("elmfat", "/", NULL);
        return res;
    }
    return -STATUS_EIO;
       
}

/** Mount a file system by path 
*  Note: You need compare the device_name with fat_fs.ops->dev_name and find the file system operator
*        then call ops->mount().
*
*  @param data: If you have mutilple file system it can be use for pass the file system object pointer save in fat_fs->data. 
*/
int fs_mount(const char* device_name, const char* path, const void* data)
{
	if(!strcmp(device_name, "elmfat")){
		strcpy(fat_fs.path, path);
		int ret = fat_fs.ops->mount(&fat_fs, data);
    return error_handle(-ret);
	}
    return -STATUS_EIO;
} 

/* Note: Before call ops->open() you may copy the path and flags parameters into fd object structure */
int file_open(struct fs_fd* fd, const char *path, int flags)
{
	fd->flags = flags;
    strcpy(fd->path, path);
    int ret = fat_fs.ops->open(fd);
    return error_handle(-ret);
}

int file_read(struct fs_fd* fd, void *buf, size_t len)
{
    int ret = fat_fs.ops->read(fd, buf, len);
    // printk("%d\n", ret);
    if(ret<0)
        return error_handle(-ret);
    return ret;
}

int file_write(struct fs_fd* fd, const void *buf, size_t len)
{
    int ret = fat_fs.ops->write(fd, buf, len);
    // printk("%d\n", ret);
    if(ret<0)
        return error_handle(-ret);
    return ret;
}

int file_close(struct fs_fd* fd)
{
    int ret = fat_fs.ops->close(fd);
    return error_handle(-ret);
}
int file_lseek(struct fs_fd* fd, off_t offset)
{
    int ret = fat_fs.ops->lseek(fd,offset);
    return error_handle(-ret);
}
int file_unlink(const char *path)
{
    int ret = fat_fs.ops->unlink(path);
    // printk("%d\n",ret);
    if(!ret){
        int i;
        for(i = 0 ; i < FS_FD_MAX ; i++){
            if(!strcmp(fd_table[i].path, path)){
                memset(fd_table[i].path, 0, sizeof(fd_table[i].path));
                fd_table[i].ref_count = 0;
                break;
            }
        }
    }
    return error_handle(-ret);
}

int file_opendir(DIR *dir, const char *pathname)
{
    int ret = fat_fs.ops->opendir(dir, pathname);
    return error_handle(-ret);
}

int file_readdir(DIR *dir, FILINFO *file)
{
    int ret = fat_fs.ops->readdir(dir, file);
    return error_handle(-ret);
}

int file_closedir(DIR *dir)
{
    int ret = fat_fs.ops->closedir(dir);
    return error_handle(-ret);
}

int file_mkdir(const char *pathname)
{
    int ret = fat_fs.ops->mkdir(pathname);
    return error_handle(-ret);
}

/**
 * @ingroup Fd
 * This function will allocate a file descriptor.
 *
 * @return -1 on failed or the allocated file descriptor.
 */
int fd_new(void)
{
	struct fs_fd* d;
	int idx;

	/* find an empty fd entry */
	for (idx = 0; idx < FS_FD_MAX && fd_table[idx].ref_count > 0; idx++);


	/* can't find an empty fd entry */
	if (idx == FS_FD_MAX)
	{
		idx = -1;
		goto __result;
	}

	d = &(fd_table[idx]);
	d->ref_count = 1;

__result:
	return idx;
}

/**
 * @ingroup Fd
 *
 * This function will return a file descriptor structure according to file
 * descriptor.
 *
 * @return NULL on on this file descriptor or the file descriptor structure
 * pointer.
 */
struct fs_fd* fd_get(int fd)
{
	struct fs_fd* d;

	if ( fd < 0 || fd > FS_FD_MAX ) return NULL;

	d = &fd_table[fd];

	/* increase the reference count */
	d->ref_count ++;

	return d;
}

/**
 * @ingroup Fd
 *
 * This function will put the file descriptor.
 */
void fd_put(struct fs_fd* fd)
{

	fd->ref_count --;

	/* clear this fd entry */
	if ( fd->ref_count == 0 )
	{
		//memset(fd, 0, sizeof(struct fs_fd));
		memset(fd->data, 0, sizeof(FIL));
	}
};



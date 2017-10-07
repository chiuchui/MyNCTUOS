#include <inc/syscall.h>
#include <inc/trap.h>

#define SYSCALL_NOARG(name, ret_t) \
   ret_t name(void) { return syscall((SYS_##name), 0, 0, 0, 0, 0); }

#define SYSCALL_1ARG(name, ret_t, typ_arg1) \
ret_t name(typ_arg1 a1) { return syscall((SYS_##name), (uint32_t)a1, 0, 0, 0, 0); }

#define SYSCALL_2ARG(name, ret_t, typ_arg1,typ_arg2) \
ret_t name(typ_arg1 a1, typ_arg2 a2) { return syscall((SYS_##name), (uint32_t)a1, (uint32_t)a2, 0, 0, 0); }

#define SYSCALL_3ARG(name, ret_t, typ_arg1, typ_arg2, typ_arg3) \
ret_t name(typ_arg1 a1, typ_arg2 a2, typ_arg3 a3) { return syscall((SYS_##name), (uint32_t)a1, (uint32_t)a2, (uint32_t)a3, 0, 0); }

#define SYSCALL_4ARG(name, ret_t, typ_arg1, typ_arg2, typ_arg3, typ_arg4) \
ret_t name(typ_arg1 a1, typ_arg2 a2, typ_arg3 a3, typ_arg4 a4) { return syscall((SYS_##name), (uint32_t)a1, (uint32_t)a2, (uint32_t)a3, (uint32_t)a4, 0); }

static inline int32_t
syscall(int num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "d" (a1),
		  "c" (a2),
		  "b" (a3),
		  "D" (a4),
		  "S" (a5)
		: "cc", "memory");

	return ret;
}

//***********Lab7 syscalls***********//
SYSCALL_1ARG(close, int, int)
SYSCALL_3ARG(open, int, const char *, int, int)
SYSCALL_3ARG(read, int, int, void *, size_t)
SYSCALL_3ARG(write, int, int, const void *, size_t)
SYSCALL_3ARG(lseek, off_t, int, off_t, int)
SYSCALL_1ARG(unlink, int, const char *)
SYSCALL_2ARG(opendir, int, DIR *, const char *)
SYSCALL_2ARG(readdir, int, DIR *, FILINFO *)
SYSCALL_1ARG(closedir, int, DIR *)
SYSCALL_1ARG(mkdir, int, const char *)
/////////////////////////////
SYSCALL_NOARG(getc, int)
SYSCALL_NOARG(getcid, int32_t)

void
puts(const char *s, size_t len)
{
	syscall(SYS_puts,(uint32_t)s, len, 0, 0, 0);
}

/* TODO: Lab 5
 * Please add interface needed for 
 * sleep, settextcolor, kill_self, fork, getpid, cls, get_num_free_page,
 * and get_num_used_page
 * Those syscall interface are needed by shell.c to implement basic 
 * function of the shell
 *
 * HINT: You can use SYSCALL_NOARG to save your time.
 */
void sleep(uint32_t ticks)
{
	syscall(SYS_sleep,(uint32_t)ticks, 0, 0, 0, 0);
}

void kill_self()
{
	int pid = getpid();
	syscall(SYS_kill,(uint32_t)pid, 0, 0, 0, 0);
}

void settextcolor(unsigned char forecolor, unsigned char backcolor)
{
	syscall(SYS_settextcolor,(uint32_t)forecolor, backcolor, 0, 0, 0);
}

// int32_t get_num_used_page(void)
SYSCALL_NOARG(get_num_used_page, int)

// int32_t cls(void);
SYSCALL_NOARG(cls, int32_t)

// int32_t get_num_free_page(void);
SYSCALL_NOARG(get_num_free_page, int)


// unsigned long get_ticks(void);
SYSCALL_NOARG(get_ticks, unsigned long)

// int32_t fork(void);
SYSCALL_NOARG(fork, int32_t)

// int32_t getpid(void);
SYSCALL_NOARG(getpid, int32_t)
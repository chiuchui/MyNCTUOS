#include <inc/mmu.h>
#include <inc/types.h>
#include <inc/string.h>
#include <inc/x86.h>
#include <inc/memlayout.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/cpu.h>
#include <kernel/spinlock.h>

// Global descriptor table.
//
// Set up global descriptor table (GDT) with separate segments for
// kernel mode and user mode.  Segments serve many purposes on the x86.
// We don't use any of their memory-mapping capabilities, but we need
// them to switch privilege levels. 
//
// The kernel and user segments are identical except for the DPL.
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
// In particular, the last argument to the SEG macro used in the
// definition of gdt specifies the Descriptor Privilege Level (DPL)
// of that descriptor: 0 for kernel and 3 for user.
//
struct Segdesc gdt[NCPU + 5] =
{
	// 0x0 - unused (always faults -- for trapping NULL far pointers)
	SEG_NULL,

	// 0x8 - kernel code segment
	[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x10 - kernel data segment
	[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

	// 0x18 - user code segment
	[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x20 - user data segment
	[GD_UD >> 3] = SEG(STA_W , 0x0, 0xffffffff, 3),

	// First TSS descriptors (starting from GD_TSS0) are initialized
	// in task_init()
	[GD_TSS0 >> 3] = SEG_NULL
	
};

struct Pseudodesc gdt_pd = {
	sizeof(gdt) - 1, (unsigned long) gdt
};



static struct tss_struct tss;
Task tasks[NR_TASKS];

extern char bootstack[];

extern char UTEXT_start[], UTEXT_end[];
extern char UDATA_start[], UDATA_end[];
extern char UBSS_start[], UBSS_end[];
extern char URODATA_start[], URODATA_end[];
/* Initialized by task_init */
uint32_t UTEXT_SZ;
uint32_t UDATA_SZ;
uint32_t UBSS_SZ;
uint32_t URODATA_SZ;

Task *cur_task = NULL; //Current running task

struct spinlock tasks_lock;
extern void sched_yield(void);


/* TODO: Lab5
 * 1. Find a free task structure for the new task,
 *    the global task list is in the array "tasks".
 *    You should find task that is in the state "TASK_FREE"
 *    If cannot find one, return -1.
 *
 * 2. Setup the page directory for the new task
 *
 * 3. Setup the user stack for the new task, you can use
 *    page_alloc() and page_insert(), noted that the va
 *    of user stack is started at USTACKTOP and grows down
 *    to USR_STACK_SIZE, remember that the permission of
 *    those pages should include PTE_U
 *
 * 4. Setup the Trapframe for the new task
 *    We've done this for you, please make sure you
 *    understand the code.
 *
 * 5. Setup the task related data structure
 *    You should fill in task_id, state, parent_id,
 *    and its schedule time quantum (remind_ticks).
 *
 * 6. Return the pid of the newly created task.
 
 */
int task_create()
{
	Task *ts = NULL;

	/* Find a free task structure */
    spin_lock(&tasks_lock);
	int i;
	for (i = 0; i < NR_TASKS; i++)
	{
		if(tasks[i].state == TASK_FREE)
		{
			ts = &tasks[i];
			ts->task_id = i;
			break;
		}
	}
	if (i==NR_TASKS-1)
	{
		spin_unlock(&tasks_lock);
		return -1;
	}
	/* Setup Page Directory and pages for kernel*/
	if (!(ts->pgdir = setupkvm()))
		panic("Not enough memory for per process page directory!\n");

	/* Setup User Stack */
	for(i=USTACKTOP-USR_STACK_SIZE; i<USTACKTOP; i+=PGSIZE)
	{
		struct PageInfo *u_stack = page_alloc(ALLOC_ZERO);
		page_insert(ts->pgdir, u_stack, i, PTE_U|PTE_W|PTE_P);
	}
	/* Setup Trapframe */
	memset( &(ts->tf), 0, sizeof(ts->tf));

	ts->tf.tf_cs = GD_UT | 0x03;
	ts->tf.tf_ds = GD_UD | 0x03;
	ts->tf.tf_es = GD_UD | 0x03;
	ts->tf.tf_ss = GD_UD | 0x03;
	ts->tf.tf_esp = USTACKTOP-PGSIZE;

	/* Setup task structure (task_id and parent_id) */
	// int task_id;
	// int parent_id;
	// struct Trapframe tf; //Saved registers
	// int32_t remind_ticks;
	if(thiscpu->cpu_task==NULL)
		ts->parent_id = 0;
	else
		ts->parent_id = thiscpu->cpu_task->parent_id;
	ts->remind_ticks = TIME_QUANT;
	ts->state = TASK_RUNNABLE;
    spin_unlock(&tasks_lock);
	return ts->task_id;
}


/* TODO: Lab5
 * This function free the memory allocated by kernel.
 *
 * 1. Be sure to change the page directory to kernel's page
 *    directory to avoid page fault when removing the page
 *    table entry.
 *    You can change the current directory with lcr3 provided
 *    in inc/x86.h
 *
 * 2. You have to remove pages of USER STACK
 *
 * 3. You have to remove pages of page table
 *
 * 4. You have to remove pages of page directory
 *
 * HINT: You can refer to page_remove, ptable_remove, and pgdir_remove
 */


static void task_free(int pid)
{
	Task *ts = NULL;
	int i=0;
	for (i = 0; i < NR_TASKS; i++)
	{
		if(tasks[i].task_id == pid)
		{
			ts = &tasks[i];
			break;
		}
	}

	// extern pde_t *kern_pgdir;
	lcr3(PADDR(kern_pgdir));
	
	/*remove pages of USER STACK */
	for(i=USTACKTOP-USR_STACK_SIZE; i<USTACKTOP; i+=PGSIZE)
	{
		page_remove(ts->pgdir, i);
	}

	/*remove pages of page table*/
	ptable_remove(ts->pgdir);

	/*remove pages of page directory*/
	pgdir_remove(ts->pgdir);

}

// Lab6 TODO
//
// Modify it so that the task will be removed form cpu runqueue
// ( we not implement signal yet so do not try to kill process
// running on other cpu )
//
void sys_kill(int pid)
{
	if (pid > 0 && pid < NR_TASKS)
	{
	/* TODO: Lab 5
   * Remember to change the state of tasks
   * Free the memory
   * and invoke the scheduler for yield
   */
		if(thiscpu->cpu_id != tasks[pid].cpu_id)
			return;
		if(thiscpu->cpu_rq.total > 1)
		{
			int i=0;
			for(i;i<thiscpu->cpu_rq.total;i++)
			{
				if(thiscpu->cpu_rq.runq[i]==pid)
				{
					int last = thiscpu->cpu_rq.runq[ thiscpu->cpu_rq.total-1 ];
					thiscpu->cpu_rq.runq[i] = last;
					thiscpu->cpu_rq.total--;
					break;
				}
			}
		}
		spin_lock(&tasks_lock);
		tasks[pid].state = TASK_FREE;
		task_free(pid);
		spin_unlock(&tasks_lock);
		sched_yield();
	}
}

/* TODO: Lab 5
 * In this function, you have several things todo
 *
 * 1. Use task_create() to create an empty task, return -1
 *    if cannot create a new one.
 *
 * 2. Copy the trap frame of the parent to the child
 *
 * 3. Copy the content of the old stack to the new one,
 *    you can use memcpy to do the job. Remember all the
 *    address you use should be virtual address.
 *
 * 4. Setup virtual memory mapping of the user prgram 
 *    in the new task's page table.
 *    According to linker script, you can determine where
 *    is the user program. We've done this part for you,
 *    but you should understand how it works.
 *
 * 5. The very important step is to let child and 
 *    parent be distinguishable!
 *
 * HINT: You should understand how system call return
 * it's return value.
 */

//
// Lab6 TODO:
//
// Modify it so that the task will disptach to different cpu runqueue
// (please try to load balance, don't put all task into one cpu)
//
int sys_fork()
{
	/* pid for newly created process */
	static int lastcpu = 0;

	int pid;
	pid = task_create();
	if(pid ==-1)
		return -1;
	/* Step 2:Copy the trap frame of the parent to the child*/
	memcpy(&tasks[pid].tf, &thiscpu->cpu_task->tf, sizeof(struct Trapframe));
	/* Step 3:Copy the content of the old stack to the new one*/
	int i;
	for(i=USTACKTOP-USR_STACK_SIZE; i<USTACKTOP; i+=PGSIZE)
	{
		//find old stack physical address
		pte_t* old_stack = pgdir_walk(thiscpu->cpu_task->pgdir, i, 0);
		if (old_stack==NULL | (!(*old_stack & PTE_P)))
		{
			cprintf("sys_fork has error because old stack does not exist.\n");
			break;
		}
		physaddr_t old_stack_addr = PTE_ADDR(*old_stack);

		//find new stack physical address
		pte_t* new_stack = pgdir_walk(tasks[pid].pgdir, i, 0);
		if (new_stack==NULL | (!(*new_stack & PTE_P)))
		{
			cprintf("sys_fork has error because new stack does not exist.\n");
			break;
		}
		physaddr_t new_stack_addr = PTE_ADDR(*new_stack);

		//copy
		
		memcpy((char*)KADDR(new_stack_addr), (char*)KADDR(old_stack_addr), PGSIZE);
	}

	if ((uint32_t)thiscpu->cpu_task)
	{
		/* Step 4: All user program use the same code for now */
		setupvm(tasks[pid].pgdir, (uint32_t)UTEXT_start, UTEXT_SZ);
		setupvm(tasks[pid].pgdir, (uint32_t)UDATA_start, UDATA_SZ);
		setupvm(tasks[pid].pgdir, (uint32_t)UBSS_start, UBSS_SZ);
		setupvm(tasks[pid].pgdir, (uint32_t)URODATA_start, URODATA_SZ);

		/*Step 5: Return value*/
		tasks[pid].tf.tf_regs.reg_eax = 0;
		thiscpu->cpu_task->tf.tf_regs.reg_eax = pid;
	}
	spin_lock(&tasks_lock);
    lastcpu = (lastcpu+1) % ncpu;
    tasks[pid].cpu_id = lastcpu;
    cpus[lastcpu].cpu_rq.runq[cpus[lastcpu].cpu_rq.total] = pid;
    cpus[lastcpu].cpu_rq.total++;
	spin_unlock(&tasks_lock);
	return pid;
}

/* TODO: Lab5
 * We've done the initialization for you,
 * please make sure you understand the code.
 */
void task_init()
{
	spin_initlock(&tasks_lock);
	extern int user_entry();
	int i;
	UTEXT_SZ = (uint32_t)(UTEXT_end - UTEXT_start);
	UDATA_SZ = (uint32_t)(UDATA_end - UDATA_start);
	UBSS_SZ = (uint32_t)(UBSS_end - UBSS_start);
	URODATA_SZ = (uint32_t)(URODATA_end - URODATA_start);

	/* Initial task sturcture */
	for (i = 0; i < NR_TASKS; i++)
	{
		memset(&(tasks[i]), 0, sizeof(Task));
		tasks[i].state = TASK_FREE;

	}
	task_init_percpu();
}

// Lab6 TODO
//
// Please modify this function to:
//
// 1. init idle task for non-booting AP 
//    (remember to put the task in cpu runqueue) 
//
// 2. init per-CPU Runqueue
//
// 3. init per-CPU system registers
//
// 4. init per-CPU TSS
//
void task_init_percpu()
{
	int i;
	static flag = 1;
	extern int user_entry();
	extern int idle_entry();
	int j=cpunum();
	
	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	memset(&(cpus[j].cpu_tss), 0, sizeof(cpus[j].cpu_tss));
	cpus[j].cpu_tss.ts_esp0 = (uint32_t)percpu_kstacks[cpus[j].cpu_id] + KSTKSIZE;
	cpus[j].cpu_tss.ts_ss0 = GD_KD;

	// fs and gs stay in user data segment
	cpus[j].cpu_tss.ts_fs = GD_UD | 0x03;
	cpus[j].cpu_tss.ts_gs = GD_UD | 0x03;

	/* Setup TSS in GDT */
	gdt[(GD_TSS0 >> 3) + j] = SEG16(STS_T32A, (uint32_t)(&cpus[j].cpu_tss), sizeof(struct tss_struct), 0);
	gdt[(GD_TSS0 >> 3) + j].sd_s = 0;
	/* Setup first task */
	i = task_create();
	cpus[j].cpu_task = &(tasks[i]);

	/* For user program */
	setupvm(cpus[j].cpu_task->pgdir, (uint32_t)UTEXT_start, UTEXT_SZ);
	setupvm(cpus[j].cpu_task->pgdir, (uint32_t)UDATA_start, UDATA_SZ);
	setupvm(cpus[j].cpu_task->pgdir, (uint32_t)UBSS_start, UBSS_SZ);
	setupvm(cpus[j].cpu_task->pgdir, (uint32_t)URODATA_start, URODATA_SZ);
// <<<<<<< HEAD
// =======
	if(flag)
	{
		cpus[j].cpu_task->tf.tf_eip = (uint32_t)user_entry;
		cpus[j].cpu_task->state = TASK_RUNNING;
		flag=0;
	}
	else
	{
		cpus[j].cpu_task->tf.tf_eip = (uint32_t)idle_entry;
		cpus[j].cpu_task->state = TASK_RUNNING;
	}
	
	/* Setup run queue */
	memset(&(cpus[j].cpu_rq), 0, sizeof(cpus[j].cpu_rq));
	cpus[j].cpu_rq.runq[0] = i;
	cpus[j].cpu_rq.current_index = 0;
	cpus[j].cpu_rq.total = 1;

	/* Load GDT&LDT */
	lgdt(&gdt_pd);
	lldt(0);

	// Load the TSS selector 
	ltr((GD_TSS0 & 0x3) | (((GD_TSS0 >> 3) + j) << 3) );
}
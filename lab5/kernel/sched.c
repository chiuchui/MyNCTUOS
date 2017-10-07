#include <kernel/task.h>
#include <inc/x86.h>

#define ctx_switch(ts) \
  do { env_pop_tf(&((ts)->tf)); } while(0)

/* TODO: Lab5
* Implement a simple round-robin scheduler (Start with the next one)
*
* 1. You have to remember the task you picked last time.
*
* 2. If the next task is in TASK_RUNNABLE state, choose
*    it.
*
* 3. After your choice set cur_task to the picked task
*    and set its state, remind_ticks, and change page
*    directory to its pgdir.
*
* 4. CONTEXT SWITCH, leverage the macro ctx_switch(ts)
*    Please make sure you understand the mechanism.
*/
void sched_yield(void)
{

	extern Task tasks[];
	extern Task *cur_task;
	int id = cur_task->task_id;
	int i=0;
	for(i=1;i<=NR_TASKS;i++)
	{
		if(tasks[(id+i)%NR_TASKS].state == TASK_RUNNABLE)
		{
			id = (id+i)%NR_TASKS;
			break;
		}
	}

	if(id == cur_task->task_id)
	{
		cur_task->remind_ticks = TIME_QUANT;
	}
	else if(cur_task->state == TASK_RUNNING)
	{
		cur_task->state = TASK_RUNNABLE;
		cur_task->remind_ticks = TIME_QUANT;
		cur_task = &(tasks[id]);
		cur_task->state = TASK_RUNNING;
	    lcr3(PADDR(cur_task->pgdir));
		ctx_switch(&tasks[id]);
	}
	else
	{
		cur_task = &(tasks[id]);
		cur_task->state = TASK_RUNNING;
	    lcr3(PADDR(cur_task->pgdir));
		ctx_switch(&tasks[id]);
	}
}
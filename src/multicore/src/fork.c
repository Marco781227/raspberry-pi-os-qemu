#include "fork.h"
#include "entry.h"
#include "mm.h"
#include "sched.h"
#include "utils.h"

int copy_process(unsigned long clone_flags, unsigned long fn,
                 unsigned long arg) {
    unsigned char core_id = get_core_id();
    preempt_disable(core_id);
    struct task_struct *p;

    unsigned long page = allocate_kernel_page();
    p = (struct task_struct *)page;
    struct pt_regs *childregs = task_pt_regs(p);

    if (!p)
        return -1;

    if (clone_flags & PF_KTHREAD) {
        p->cpu_context.x19 = fn;
        p->cpu_context.x20 = arg;
    } else {
        struct pt_regs *cur_regs = task_pt_regs(currents[core_id]);
        *cur_regs = *childregs;
        childregs->regs[0] = 0;
        copy_virt_memory(p);
    }
    p->flags = clone_flags;
    p->priority = currents[core_id]->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1; // disable preemtion until schedule_tail

    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)childregs;
    int pid = nr_tasks++;
    task[pid] = p;

    preempt_enable(core_id);
    return pid;
}

/* @start: a pointer to the beginning of the user code (to be copied to the new
   task),
   @size: size of the area
   @pc: offset of the startup function inside the area
*/

int move_to_user_mode(unsigned long start, unsigned long size,
                      unsigned long pc) {
    unsigned char core_id = get_core_id();
    struct pt_regs *regs = task_pt_regs(currents[core_id]);
    regs->pstate = PSR_MODE_EL0t;
    regs->pc = pc;
    regs->sp = 2 * PAGE_SIZE;
    unsigned long code_page = allocate_user_page(currents[core_id], 0);
    if (code_page == 0) {
        return -1;
    }
    memcpy(start, code_page, size); /* NB: arg1-src; arg2-dest */
    set_pgd(currents[core_id]->mm.pgd);
    return 0;
}

struct pt_regs *task_pt_regs(struct task_struct *tsk) {
    unsigned long p = (unsigned long)tsk + THREAD_SIZE - sizeof(struct pt_regs);
    return (struct pt_regs *)p;
}

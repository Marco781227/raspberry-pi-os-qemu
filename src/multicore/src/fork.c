#include "fork.h"
#include "entry.h"
#include "mm.h"
#include "sched.h"
#include "utils.h"
#include "spinlock.h"
#include "printf.h"

int copy_process(unsigned long clone_flags,
                 unsigned long fn,
                 unsigned long arg)
{
    struct runqueue *rq = this_rq();
    struct task_struct *current = rq->current;

    preempt_disable();

    unsigned long page = allocate_kernel_page();
    if (!page) {
        preempt_enable();
        return -1;
    }

    struct task_struct *p = (struct task_struct *)page;
    struct pt_regs *childregs = task_pt_regs(p);

    /* Copie du contexte */
    if (clone_flags & PF_KTHREAD) {
        p->cpu_context.x19 = fn;
        p->cpu_context.x20 = arg;
    } else {
        struct pt_regs *cur_regs = task_pt_regs(current);
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        copy_virt_memory(p);
    }

    /* Initialisation de la task */
    p->flags = clone_flags;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1;   /* pas préemptable avant premier schedule */
    p->cpu = current->cpu;

    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)childregs;

    /* Insertion dans la runqueue locale */
    p->next = rq->task_list;
    rq->task_list = p;

    preempt_enable();
    return 0;   /* pas encore de PID global */
}

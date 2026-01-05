#include "fork.h"
#include "entry.h"
#include "mm.h"
#include "sched.h"
#include "utils.h"
#include "spinlock.h"
#include "printf.h"


// returns the runqueue of the current core
static inline struct runqueue *this_rq(void)
{
    return &runqueues[get_core_id()];
}

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

    if (clone_flags & PF_KTHREAD) {
        p->cpu_context.x19 = fn;
        p->cpu_context.x20 = arg;
    } else {
        struct pt_regs *cur_regs = task_pt_regs(current);
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        copy_virt_memory(p);
    }

    p->flags = clone_flags;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1;
    p->cpu = current->cpu;

    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)childregs;

    p->next = rq->task_list;
    rq->task_list = p;

    preempt_enable();
    return 0;
}

int move_to_user_mode(unsigned long start,
                      unsigned long size,
                      unsigned long pc)
{
    struct task_struct *current = this_rq()->current;
    struct pt_regs *regs = task_pt_regs(current);

    regs->pstate = PSR_MODE_EL0t;
    regs->pc = pc;
    regs->sp = 2 * PAGE_SIZE;

    unsigned long code_page = allocate_user_page(current, 0);
    if (!code_page)
        return -1;

    memcpy(start, code_page, size);
    set_pgd(current->mm.pgd);
    return 0;
}

struct pt_regs *task_pt_regs(struct task_struct *tsk) {
  unsigned long p = (unsigned long)tsk + THREAD_SIZE - sizeof(struct pt_regs);
  return (struct pt_regs *)p;
}


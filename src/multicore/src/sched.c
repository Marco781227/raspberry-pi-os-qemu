#include "sched.h"
#include "irq.h"
#include "utils.h"
#include <printf.h>
#include "spinlock.h"
#include <stddef.h>

static struct task_struct init_task_0 = INIT_TASK;
static struct task_struct init_task_1 = INIT_TASK;
static struct task_struct init_task_2 = INIT_TASK;
static struct task_struct init_task_3 = INIT_TASK;

struct runqueue runqueues[NB_CPU];


// returns the runqueue of the current core
static inline struct runqueue *this_rq(void)
{
    return &runqueues[get_core_id()];
}

void sched_init(void)
{
    printf("Initiate scheduler\n");
    struct task_struct *init_tasks[NB_CPU] = {
        &init_task_0, &init_task_1, &init_task_2, &init_task_3
    };

    for (int cpu = 0; cpu < NB_CPU; cpu++) {
        struct runqueue *rq = &runqueues[cpu];

        //spin_lock_init(&rq->lock); no locks for now

        rq->idle = init_tasks[cpu];
        rq->current = rq->idle;
        rq->task_list = rq->idle;

        rq->idle->cpu = cpu;
        rq->idle->state = TASK_RUNNING;
        rq->idle->counter = 0;
        rq->idle->next = NULL;
    }
}

void preempt_disable(void) {
    this_rq()->current->preempt_count++;
}

void preempt_enable(void) {
    this_rq()->current->preempt_count--;
}

void _schedule(void) {
    struct runqueue *rq = this_rq();
    struct task_struct *p, *next = rq->idle;
    int c = 0;

    printf("Core %d : SCHEDULE -- Disabling preemt of task : %d with counter : %d\n",p->cpu, p, p->counter);
    preempt_disable();
    
    for (p = rq->task_list; p; p = p->next) {
        if (p->state == TASK_RUNNING && p->counter > c) {
            c = p->counter;
            next = p;
        }
    }
    
    if (c == 0) {
        for (p = rq->task_list; p; p = p->next)
            p->counter = (p->counter >> 1) + p->priority;
    }

    if (rq->current != next)
        switch_to(next);

    printf("Core %d : SCHEDULE -- Enabling preemt of task : %d with counter : %d\n",p->cpu, p, p->counter);
    preempt_enable();
}

void schedule(void)
{
    this_rq()->current->counter = 0;
    _schedule();
}

void switch_to(struct task_struct *next) {
    struct runqueue *rq = this_rq();
    struct task_struct *prev = rq->current;

    if (prev == next)
        return;

    rq->current = next;
    set_pgd(next->mm.pgd);

    printf("Core %d : switch %p -> %p\n",
           get_core_id(), prev, next);

    cpu_switch_to(prev, next);
}

void timer_tick(void)
{
    struct runqueue *rq = this_rq();
    struct task_struct *curr = rq->current;

    curr->counter--;

    printf("Core %d : tick task=%p counter=%ld preempt=%ld\n",
           get_core_id(), curr, curr->counter, curr->preempt_count);

    if (curr->counter > 0 || curr->preempt_count > 0)
        return;

    curr->counter = 0;
    enable_irq();
    _schedule();
    disable_irq();
}

void exit_process(void)
{
    struct runqueue *rq = this_rq();
    struct task_struct *p = rq->current;

    preempt_disable();
    p->state = TASK_ZOMBIE;
    preempt_enable();

    schedule();
}

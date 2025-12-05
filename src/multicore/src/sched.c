#include "sched.h"
#include "irq.h"
#include "utils.h"

static struct task_struct init_task = INIT_TASK;
struct task_struct *currents[NB_CPU] = {[0] = &(init_task)};
struct task_struct *task[NR_TASKS] = {
    &(init_task),
};
int nr_tasks = 1;

void preempt_disable(unsigned char core_id) {
    currents[core_id]->preempt_count++;
}

void preempt_enable(unsigned char core_id) {
    currents[core_id]->preempt_count--;
}

void _schedule(unsigned char core_id) {
    preempt_disable(core_id);
    int next, c;
    struct task_struct *p;
    while (1) {
        c = -1;
        next = 0;
        for (int i = 0; i < NR_TASKS; i++) {
            p = task[i];
            if (p && p->state == TASK_RUNNING && p->counter > c) {
                c = p->counter;
                next = i;
            }
        }
        if (c) {
            break;
        }
        for (int i = 0; i < NR_TASKS; i++) {
            p = task[i];
            if (p) {
                p->counter = (p->counter >> 1) + p->priority;
            }
        }
    }
    switch_to(task[next]);
    preempt_enable(core_id);
}

void schedule(unsigned char core_id) {
    currents[core_id]->counter = 0;
    _schedule(core_id);
}

void switch_to(struct task_struct *next) {
    unsigned char core_id = get_core_id();
    if (currents[core_id] == next)
        return;
    struct task_struct *prev = currents[core_id];
    currents[core_id] = next;
    set_pgd(next->mm.pgd);
    cpu_switch_to(prev, next);
}

void schedule_tail(void) {
    unsigned char core_id = get_core_id();
    preempt_enable(core_id);
}

void timer_tick() {
    unsigned char core_id = get_core_id();
    --currents[core_id]->counter;
    if (currents[core_id]->counter > 0 ||
        currents[core_id]->preempt_count > 0) {
        return;
    }
    currents[core_id]->counter = 0;
    enable_irq();
    _schedule(core_id);
    disable_irq();
}

void exit_process() {
    unsigned char core_id = get_core_id();
    preempt_disable(core_id);
    for (int i = 0; i < NR_TASKS; i++) {
        if (task[i] == currents[core_id]) {
            task[i]->state = TASK_ZOMBIE;
            break;
        }
    }
    preempt_enable(core_id);
    schedule(core_id);
}

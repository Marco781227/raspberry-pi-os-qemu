#include "sched.h"
#include "irq.h"
#include "utils.h"
#include <printf.h>
#include "spinlock.h"

static struct task_struct init_task_0 = INIT_TASK;
static struct task_struct init_task_1 = INIT_TASK;
static struct task_struct init_task_2 = INIT_TASK;
static struct task_struct init_task_3 = INIT_TASK;
struct task_struct *currents[NB_CPU] = { &(init_task_0), &(init_task_1), &(init_task_2), &(init_task_3)};
struct task_struct *task[NR_TASKS] = {
    &(init_task_0),
    &(init_task_1),
    &(init_task_2),
    &(init_task_3),
};
int nr_tasks = NB_CPU;

void preempt_disable(unsigned char core_id) {
    currents[core_id]->preempt_count++;
}

void preempt_enable(unsigned char core_id) {
    currents[core_id]->preempt_count--;
}

void _schedule(unsigned char core_id) {
    printf("Core %d : SCHEDULE -- Disabling preemt of task : %d with counter : %d\n",core_id, currents[core_id], currents[core_id]->counter);
    preempt_disable(core_id);
    printf("Core %d : Arrived in _schedule\n",core_id);
    int next, c;
    struct task_struct *p;
    c = 0;
    next = core_id;
    for (int i = NB_CPU; i < NR_TASKS; i++) {
        p = task[i];
        if (p && p->state == TASK_RUNNING && p->counter > c && !p->taken) {
            c = p->counter;
            next = i;
        }
    }
    if (c == 0) {
      for (int i = 0; i < NR_TASKS; i++) {
          p = task[i];
          if (p && !p->taken) {
              p->counter = (p->counter >> 1) + p->priority;
          }
      }
    }
    else {
      task[next]->taken = 1;
    }
    switch_to(task[next]);
    core_id = get_core_id();
    printf("Core %d : SCHEDULE -- Enabling preemt of task : %d with counter : %d\n",core_id, currents[core_id], currents[core_id]->counter);
    preempt_enable(core_id);
}

void schedule(unsigned char core_id) {
    currents[core_id]->counter = 0;
    _schedule(core_id);
}

void switch_to(struct task_struct *next) {
    unsigned char core_id = get_core_id();
    printf("Core %d : Exiting switch-to\n",core_id);
    if (currents[core_id] == next)
        return;

    struct task_struct *prev = currents[core_id];
    prev->taken = 0;
    currents[core_id] = next;
    set_pgd(next->mm.pgd);

    printf("Core %d : Taking next, Prev : %d, Next : %d\n",core_id, prev, next);
    cpu_switch_to(prev, next);
    core_id = get_core_id();
    printf("Core %d : Returning from cpu_switch_to, Prev : %d, Next : %d\n",core_id, prev, next);
}

void schedule_tail(void) {
    unsigned char core_id = get_core_id();
    printf("Core %d : TAIL -- Enabling preemt of task : %d\n",core_id, currents[core_id]);
    preempt_enable(core_id);
}

void timer_tick() {
    unsigned char core_id = get_core_id();
    --currents[core_id]->counter;
    printf("Core %d : Arrived in timer_tick, task : %d counter : %d, preemp : %d\n",core_id, currents[core_id], currents[core_id]->counter, currents[core_id]->preempt_count);
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

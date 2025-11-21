#include "mm.h"
#include "sched.h"
#include "fork.h"
#include "utils.h"
#include "entry.h"
#include "timer.h"
#include "peripherals/irq.h"


void init_lancement_coeur_secondaire()
{
    int id_core = get_core_id();

    irq_vector_init();

    generic_timer_init();
    put32(TIMER_INT_CTRL_0 + id_core * 4 , TIMER_INT_CTRL_0_VALUE);

    enable_irq();


    while (1) {
        asm("wfi");
    }
}
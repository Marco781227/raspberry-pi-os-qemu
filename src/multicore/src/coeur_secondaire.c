#include "mm.h"
#include "sched.h"
#include "fork.h"
#include "utils.h"
#include "entry.h"
#include "timer.h"
#include "peripherals/irq.h"
#include "printf.h"
#include "coeur_secondaire.h"
#include "irq.h"


void init_coeur_secondaire(void)
{
    int id_core = get_core_id();

    irq_vector_init();

    generic_timer_init();
    put32(TIMER_INT_CTRL_0 + id_core * 4 , TIMER_INT_CTRL_0_VALUE);

    enable_irq();

    printf("Coeur %d inti \n", id_core);
}
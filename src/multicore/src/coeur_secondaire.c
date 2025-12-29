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

#include <stddef.h>
#include <stdint.h>

#include "mini_uart.h"
#include "spinlock.h"
#include "sys.h"
#include "user.h"
#include "spinlock.h"


void init_coeur_secondaire(void)
{
    int id_core = get_core_id();

	//lock();

	//printf("Coeur %d \n", id_core);
    //unlock();

    irq_vector_init();

    generic_timer_init();
    lock();
    put32(TIMER_INT_CTRL_0 + id_core * 4 , TIMER_INT_CTRL_0_VALUE);
    unlock();
    enable_irq();

}
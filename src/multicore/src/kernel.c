#include <stddef.h>
#include <stdint.h>
#include "fork.h"
#include "irq.h"
#include "mini_uart.h"
#include "printf.h"
#include "sched.h"
#include "spinlock.h"
#include "timer.h"
#include "user.h"
#include "utils.h"

void kernel_process() {
    printf("Kernel process started. EL %d\r\n", get_el());
    unsigned long begin = (unsigned long)&user_begin;
    unsigned long end = (unsigned long)&user_end;
    unsigned long process = (unsigned long)&user_process;
    int err = move_to_user_mode(begin, end - begin, process - begin);
    if (err < 0) {
        printf("Error while moving process to user mode\n\r");
    }
}

void kernel_main() {
    unsigned char core_id = get_core_id();
    // Core 0 initializes the uart and IRQ controller before waiting for the other cores to start
    if (core_id == 0){
      uart_init();
      init_printf(NULL, putc);
      printf("kernel boots ...\n\r");
      enable_interrupt_controller();
      int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0);
      if (res < 0) {
          printf("error while starting kernel process");
          return;
      }
      start_cores();
      printf("Starting other cores...\n\r");
    }
    irq_vector_init();
    generic_timer_init();
    enable_irq();

    printf("Core %d : Successfully started\n", core_id);
    while(1){
      printf("Core %d : Just woke up, looking for tasks to execute\n", get_core_id());
      schedule(core_id);
      asm("wfi");
    }
}

#include "utils.h"
#include "printf.h"
#include "timer.h"
#include "entry.h"
#include "peripherals/irq.h"
#include "spinlock.h"

const char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",
	"FIQ_INVALID_EL1t",
	"ERROR_INVALID_EL1T",

	"SYNC_INVALID_EL1h",
	"IRQ_INVALID_EL1h",
	"FIQ_INVALID_EL1h",
	"ERROR_INVALID_EL1h",

	"SYNC_INVALID_EL0_64",
	"IRQ_INVALID_EL0_64",
	"FIQ_INVALID_EL0_64",
	"ERROR_INVALID_EL0_64",

	"SYNC_INVALID_EL0_32",
	"IRQ_INVALID_EL0_32",
	"FIQ_INVALID_EL0_32",
	"ERROR_INVALID_EL0_32"
};

void enable_interrupt_controller()
{
  // Enables Core 0-3 Timers interrupt control for the generic timer
  put32(TIMER_INT_CTRL_0, TIMER_INT_CTRL_0_VALUE);
  put32(TIMER_INT_CTRL_1, TIMER_INT_CTRL_0_VALUE);
  put32(TIMER_INT_CTRL_2, TIMER_INT_CTRL_0_VALUE);
  put32(TIMER_INT_CTRL_3, TIMER_INT_CTRL_0_VALUE);
}

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address)
{
  unsigned char core_id = get_core_id();
	printf("Core %d : %s, ESR: %x, address: %x\r\n", core_id,entry_error_messages[type], esr, address);
}

void handle_irq(void)
{
  unsigned char core_id = get_core_id();
  unsigned long source_reg = INT_SOURCE_0 + (core_id * 4);
  printf("\nCore %d : IRQ \n", core_id);
  unsigned int irq = get32(source_reg);
	switch (irq) {
		case (GENERIC_TIMER_INTERRUPT):
      handle_generic_timer_irq();
			break;
		default:
	 printf("Inknown pending irq: %x\r\n", irq);
	}
  printf("Core %d : Finished\n",core_id);
}

#if 0
void handle_irq(void)
{
	unsigned int irq = get32(IRQ_PENDING_1);
	switch (irq) {
		case (SYSTEM_TIMER_IRQ_1):
			handle_timer_irq();
			break;
		default:
			printf("Inknown pending irq: %x\r\n", irq);
	}
}
#endif

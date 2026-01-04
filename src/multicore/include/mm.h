#ifndef _MM_H
#define _MM_H

#include "peripherals/base.h"
#include "sched.h"

#define VA_START           0xffff000000000000UL

#define PAGE_MASK          0xfffffffffffff000UL
#define PAGE_SHIFT         12
#define TABLE_SHIFT        9
#define SECTION_SHIFT      (PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE          (1UL << PAGE_SHIFT)
#define SECTION_SIZE       (1UL << SECTION_SHIFT)

#define LOW_MEMORY         (2 * SECTION_SIZE)
#define HIGH_MEMORY        DEVICE_BASE

#define PAGING_MEMORY      (HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES       (PAGING_MEMORY / PAGE_SIZE)

#define PTRS_PER_TABLE     (1 << TABLE_SHIFT)

#define PGD_SHIFT          (PAGE_SHIFT + 3 * TABLE_SHIFT)
#define PUD_SHIFT          (PAGE_SHIFT + 2 * TABLE_SHIFT)
#define PMD_SHIFT          (PAGE_SHIFT + TABLE_SHIFT)

#define PHYS_MEMORY_SIZE   (0x40000000 + SECTION_SIZE)
#define PG_DIR_SIZE        (4 * PAGE_SIZE)

#ifndef __ASSEMBLER__

/* Allocation de pages physiques */
unsigned long get_free_page(void);
void free_page(unsigned long p);

/* Allocation pour une tâche donnée */
unsigned long allocate_kernel_page(void);
unsigned long allocate_user_page(struct task_struct *task, unsigned long va);

/* Gestion de la mémoire virtuelle */
void map_page(struct task_struct *task, unsigned long va, unsigned long page);

/* Copier la mémoire utilisateur d'une tâche existante vers une nouvelle */
int copy_virt_memory(struct task_struct *dst);

/* Helpers mémoire */
void memzero(unsigned long src, unsigned long n);
void memcpy(unsigned long src, unsigned long dst, unsigned long n); /* arg1-src, arg2-dest */

extern unsigned long pg_dir;

#endif /* __ASSEMBLER__ */

#endif /* _MM_H */


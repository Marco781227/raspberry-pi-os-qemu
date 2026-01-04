#include "mm.h"
#include "arm/mmu.h"
#include "spinlock.h"
#include "utils.h"


// returns the runqueue of the current core
static inline struct runqueue *this_rq(void)
{
    return &runqueues[get_core_id()];
}

/* minimalist page allocation */
static unsigned short mem_map[PAGING_PAGES] = {0};

unsigned long allocate_kernel_page() {
    unsigned long page = get_free_page();
    if (!page) return 0;
    return page + VA_START;
}

unsigned long allocate_user_page(struct task_struct *task, unsigned long va) {
    unsigned long page = get_free_page();
    if (!page) return 0;
    map_page(task, va, page);
    return page + VA_START;
}

unsigned long get_free_page() {
    lock_mem();
    for (int i = 0; i < PAGING_PAGES; i++) {
        if (mem_map[i] == 0) {
            mem_map[i] = 1;
            unsigned long page = LOW_MEMORY + i * PAGE_SIZE;
            memzero(page + VA_START, PAGE_SIZE);
            unlock_mem();
            return page;
        }
    }
    unlock_mem();
    return 0;
}

void free_page(unsigned long p) {
    lock_mem();
    mem_map[(p - LOW_MEMORY) / PAGE_SIZE] = 0;
    unlock_mem();
}

/* Virtual memory helpers */
void map_table_entry(unsigned long *pte, unsigned long va, unsigned long pa) {
    unsigned long index = (va >> PAGE_SHIFT) & (PTRS_PER_TABLE - 1);
    pte[index] = pa | MMU_PTE_FLAGS;
}

unsigned long map_table(unsigned long *table, unsigned long shift,
                        unsigned long va, int *new_table) {
    unsigned long index = (va >> shift) & (PTRS_PER_TABLE - 1);
    if (!table[index]) {
        *new_table = 1;
        unsigned long next = get_free_page();
        table[index] = next | MM_TYPE_PAGE_TABLE;
        return next;
    } else {
        *new_table = 0;
        return table[index] & PAGE_MASK;
    }
}

void map_page(struct task_struct *task, unsigned long va, unsigned long page) {
    if (!task->mm.pgd) {
        task->mm.pgd = get_free_page();
        task->mm.kernel_pages[task->mm.kernel_pages_count++] = task->mm.pgd;
    }
    unsigned long pgd = task->mm.pgd;
    int new_table;

    unsigned long pud = map_table((unsigned long *)(pgd + VA_START), PGD_SHIFT, va, &new_table);
    if (new_table) task->mm.kernel_pages[task->mm.kernel_pages_count++] = pud;

    unsigned long pmd = map_table((unsigned long *)(pud + VA_START), PUD_SHIFT, va, &new_table);
    if (new_table) task->mm.kernel_pages[task->mm.kernel_pages_count++] = pmd;

    unsigned long pte = map_table((unsigned long *)(pmd + VA_START), PMD_SHIFT, va, &new_table);
    if (new_table) task->mm.kernel_pages[task->mm.kernel_pages_count++] = pte;

    map_table_entry((unsigned long *)(pte + VA_START), va, page);

    struct user_page u = { page, va };
    task->mm.user_pages[task->mm.user_pages_count++] = u;
}

/* Duplicate the current task's user pages into dst */
int copy_virt_memory(struct task_struct *dst) {
    struct task_struct *src = this_rq()->current;
    for (int i = 0; i < src->mm.user_pages_count; i++) {
        unsigned long kernel_va = allocate_user_page(dst, src->mm.user_pages[i].virt_addr);
        if (!kernel_va) return -1;
        memcpy(src->mm.user_pages[i].virt_addr, kernel_va, PAGE_SIZE);
    }
    return 0;
}

static int ind = 1;

int do_mem_abort(unsigned long addr, unsigned long esr) {
    unsigned long dfs = esr & 0b111111;
    if ((dfs & 0b111100) == 0b100) {
        unsigned long page = get_free_page();
        if (!page) return -1;
        map_page(this_rq()->current, addr & PAGE_MASK, page);
        ind++;
        if (ind > 2) return -1;
        return 0;
    }
    return -1;
}

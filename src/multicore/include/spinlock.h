#ifndef _SPINLOCK_H
#define _SPINLOCK_H
#ifndef __ASSEMBLER__

extern unsigned char mutex;
int atomic_add(int*, int);
void lock_write(void);
void unlock_write(void);
void lock_mem(void);
void unlock_mem(void);
void lock_sched(void);
void unlock_sched(void);

#endif
#endif /*_SPINLOCK_H */

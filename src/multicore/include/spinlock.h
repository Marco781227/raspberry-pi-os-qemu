#ifndef _SPINLOCK_H
#define _SPINLOCK_H
#ifndef __ASSEMBLER__
#define LOCKED 0
extern void lock_mutex(char *, char);
extern void free_mutex(char *, char);
void lock(void);
void unlock(void);
#endif
#endif /*_SPINLOCK_H */

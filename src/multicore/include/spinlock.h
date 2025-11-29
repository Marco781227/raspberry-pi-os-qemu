#ifndef _SPINLOCK_H
#define _SPINLOCK_H
#ifndef __ASSEMBLER__

extern unsigned char mutex;
void lock(void);
void unlock(void);

#endif
#endif /*_SPINLOCK_H */

#include "spinlock.h"

char mutex = 1;

void lock() {
    lock_mutex(&mutex, 0);
    return;
}

void unlock() {
    free_mutex(&mutex, 1);
    return;
}

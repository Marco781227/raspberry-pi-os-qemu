#include "spinlock.h"
// spinlock.S
.globl atomic_add
atomic_add:
  ldxr w2, [x0]
  add w2, w2, w1
  stxr w3, w2, [x0]
  cbnz w3, atomic_add
  dmb sy
  mov w0, w2
  ret

.globl lock_write
lock_write:
    adrp x10, mutex_write
    add x10, x10, #:lo12:mutex_write
    b lock

.globl lock_mem
lock_mem:
    adrp x10, mutex_mem
    add x10, x10, #:lo12:mutex_mem
    b lock

.globl lock_sched
lock_sched:
    adrp x10, mutex_sched
    add x10, x10, #:lo12:mutex_sched
    b lock

lock:
    mrs x9, mpidr_el1
    and x9, x9, #0xFF // x9 = core_id
    // Check ownership
    ldrb w11, [x10]
    cmp w11, w9
    b.eq 1f
    
spinloop:  
    ldxrb w11, [x10]
    cmp w11, #0xFF
    b.ne spinloop // Check if mutex is free
    stxrb w11, w9, [x10]
    cbnz w11, spinloop // Check memory access was exclusive
    dmb sy
    
1:  ret

.globl unlock_write
unlock_write:
    adrp x10, mutex_write
    add x10, x10, #:lo12:mutex_write
    b unlock

.globl unlock_mem
unlock_mem:
    adrp x10, mutex_mem
    add x10, x10, #:lo12:mutex_mem
    b unlock

.globl unlock_sched
unlock_sched:
    adrp x10, mutex_sched
    add x10, x10, #:lo12:mutex_sched
    b unlock

unlock:
    mrs x9, mpidr_el1
    and x9, x9, #0xFF
    
    ldrb w11, [x10]
    cmp w11, w9
    b.ne 1f
    
    dmb sy
    mov w11, #0xFF
    strb w11, [x10]
    
1:  ret

.section ".data"
.globl mutex_write
mutex_write : 
  .byte 0xFF
.globl mutex_shed
mutex_sched : 
  .byte 0xFF
.globl mutex_mem
mutex_mem : 
  .byte 0xFF


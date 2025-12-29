#include "spinlock.h"
// spinlock.S
// Mutex structure : [1:0] : CoreID of Owner; [2:1] Lock counter (free if =0)
.globl lock
lock:
    mrs x9, mpidr_el1
    and x9, x9, #0xFF           // x9 = core_id
    adrp x10, mutex
    add x10, x10, #:lo12:mutex
    
    // Check if lock already owned by core
    ldrb w11, [x10]
    cmp w11, w9
    b.eq increment_lock
    
spinloop:  
    ldxrb w11, [x10]
    cmp w11, #0xFF
    bne spinloop // Check if lock counter is 0
    stxrb w11, w9, [x10]
    cbnz w11, spinloop // Check if load and store were effectively exclusive
    dmb sy
    
increment_lock:
    ldr w11, [x10, #4]
    add w11, w11, #1 // Incrementing lock to handle embedded calls
    str w11, [x10, #4]
    ret

.globl unlock
unlock:
    mrs x9, mpidr_el1
    and x9, x9, #0xFF
    adrp x10, mutex
    add x10, x10, #:lo12:mutex
    
    // Check ownership
    ldrb w11, [x10]
    cmp w11, w9
    b.ne 1f
    
    // Decrement lock
    ldr w11, [x10, #4]
    sub w11, w11, #1 
    str w11, [x10, #4]
    cbnz w11, 1f
    dmb sy
    // Reset lock when counter reaches 0
    mov w11, #0xFF
    strb w11, [x10]
1:
  ret

.section ".data"
.align 4
.globl mutex
mutex : 
  .byte 0xFF
  .space 3
  .word 0
  

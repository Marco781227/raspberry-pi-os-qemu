#include "spinlock.h"
// spinlock.S
.globl lock
lock:
    mrs x9, mpidr_el1
    and x9, x9, #0xFF           // x9 = core_id
    adrp x10, mutex
    add x10, x10, #:lo12:mutex
    
    // Check si déjà propriétaire
    ldrb w11, [x10]
    cmp w11, w9
    b.eq 1f
    
    // Spinloop
spinloop:  
    ldxrb w11, [x10]
    cmp w11, #0xFF
    b.ne spinloop // Check si mutex libre
    stxrb w11, w9, [x10]
    cbnz w11, spinloop // Check si exclusivité valide
    dmb sy
    
1:  ret

.globl unlock
unlock:
    mrs x9, mpidr_el1
    and x9, x9, #0xFF
    adrp x10, mutex
    add x10, x10, #:lo12:mutex
    
    ldrb w11, [x10]
    cmp w11, w9
    b.ne 1f
    
    dmb sy
    mov w11, #0xFF
    strb w11, [x10]
    
1:  ret

.section ".data"
.globl mutex
mutex : 
  .byte 0xFF

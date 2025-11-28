.globl lock_mutex
lock_mutex:
  ldxrb   w2, [x0]     // Charge depuis l'adresse fournie
  cmp     w2, #0       // Si occupé, on boucle
  beq     lock_mutex
  stxrb   w2, w1, [x0] // Tente d'écrire
  cmp     w2, #0
  bne     lock_mutex   // Si échec, on boucle
  dmb     sy
  ret

.globl free_mutex
free_mutex:
  dmb     sy
  stlrb   w1, [x0] // Libèration du mutex
  ret

#include "arm/mmu.h"
#include "arm/sysregs.h"
#include "mm.h"
#include "peripherals/base.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1
	and	x0, x0,#0xFF		// Check processor id

	cbz	x0, master		// Hang for all non-primary CPU
	b	proc_hang

.globl _start_two
_start_two:

    mrs    x0, mpidr_el1
    and    x0, x0,#0xFF        // Check processor id

    adrp x2, secondary_start
    add x2, x2, #:lo12:secondary_start
    mov x1, #0xd8
    str x2, [x1,#8]!
    str x2, [x1,#8]!
    str x2, [x1,#8]!

	ret 

.globl secondary_start
secondary_start:

    // core_id = MPIDR_EL1[7:0]
    mrs x0, mpidr_el1 // MPIDR_EL1, qui contient : cluster ID numéro de core (affinity level 0)
    and x0, x0, #0xFF            // x0 = core_id (1,2,3)

    // sp = LOW_MEMORY + PAGE_SIZE * core_id
    ldr x1, =LOW_MEMORY
    ldr x2, =PAGE_SIZE
    madd x9, x0, x2, x1 // Chaque cœur a sa propre pile grâce au calcul
	mov sp, x9

    //bl init_coeur_secondaire
	b master_secondary
	
proc_hang:
	wfi
	b proc_hang

.globl master_secondary
master_secondary:

    // 1. S'assurer qu'EL1 sera en AArch64
    mrs x0, hcr_el2
    orr x0, x0, #(1 << 31)     // HCR_EL2.RW = 1
    msr hcr_el2, x0

    // 2. Préparer l'état après ERET
    mov x0, #SPSR_VALUE       // EL1h, interrupts masked
    msr spsr_el2, x0

    // 3. Adresse d'entrée EL1 secondaire
    adr x0, el1_entry_secondary
    msr elr_el2, x0

    // 4. Transition EL2 → EL1
    eret

.globl el1_entry_secondary
el1_entry_secondary:

    // Stack déjà positionnée dans secondary_start
    // ou recalculée ici si tu préfères

    adrp x0, idmap_dir
    msr ttbr0_el1, x0

    // 1. Charger les tables créées par le primaire
    adrp x0, pg_dir
    msr ttbr1_el1, x0

    // 2. Configurer MMU
    ldr x0, =(TCR_VALUE)
    msr tcr_el1, x0

    ldr x0, =(MAIR_VALUE)
    msr mair_el1, x0

    // 3. Activer le MMU
    mov x0, #SCTLR_MMU_ENABLED
    msr sctlr_el1, x0
    isb
    // après avoir activé le MMU + isb
    mrs x0, mpidr_el1
    and x0, x0, #0xFF          // core_id

    ldr x1, =LOW_MEMORY
    ldr x2, =PAGE_SIZE
    madd x3, x0, x2, x1        // offset = LOW_MEMORY + core_id*PAGE_SIZE

    mov x4, #VA_START
    add sp, x4, x3             // SP = VA_START + (LOW_MEMORY + offset)
	mov x9, sp
	and x9, x9, #~0xF
	mov sp, x9	

    // 4. Entrée kernel secondaire
	ldr	x2, =init_coeur_secondaire
    blr x2
	
	b proc_hang

master:

	ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0

	mrs x0, CurrentEL
    lsr x0, x0, #2
	cmp x0, #3
	beq el3

//	ldr	x0, =HCR_VALUE
//	msr	hcr_el2, x0
	mrs	x0, hcr_el2
	orr	x0, x0, #(1<<31)
	msr	hcr_el2, x0

	mov 	x0, #SPSR_VALUE
	msr	spsr_el2, x0

	adr	x0, el1_entry
	msr	elr_el2, x0
	eret

el3:
  ldr x0, =HCR_VALUE
  msr hcr_el2, x0

	ldr	x0, =SCR_VALUE
	msr	scr_el3, x0

	ldr	x0, =SPSR_VALUE
	msr	spsr_el3, x0

	adr	x0, el1_entry
	msr	elr_el3, x0

	eret

el1_entry:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

#ifdef USE_QEMU
// A workaround for QEMU's quirks on MMU emulation, which also showcases how __create_page_tables
// can be used.
//
// As soon as the MMU is on and CPU switches from physical addresses to virtual addresses,
// the emulated CPU seems to be still fetching next (few) instructions using the physical
// addresses of those instructions. These addresses will go through MMU for translation
// as if they are virtual addresses. Of course our kernel pgtables do not have translation
// for these addresses (TTBR1 is for translating virtual addresses at 0xffff...). That causes
// MMU to throw a Prefetch abort. (prefetch == instruction loading)
//
// Real Rpi3 hardware has no such a problem: after MMU is on, it will not fetch instructions
// at addresses calculated before MMU is on.
//
// The workaround is to set an "identity" mapping. That is, we create an additional
// pgtable tree at TTBR0 that maps all physical DRAM (0 -- PHYS_MEMORY_SIZE) to virtual
// addresses with the same values. That keeps translation going on at the switch of MMU.
//
// Cf: https://github.com/s-matyukevich/raspberry-pi-os/issues/8
// https://www.raspberrypi.org/forums/viewtopic.php?t=222408
	bl	__create_idmap
	adrp	x0, idmap_dir
	msr	ttbr0_el1, x0
#endif

	bl 	__create_page_tables

	mov	x0, #VA_START
	add	sp, x0, #LOW_MEMORY

	adrp	x0, pg_dir
	msr	ttbr1_el1, x0

	// tcr_el1: Translation Control Register, responsible for configuring MMU, e.g. page size
	ldr	x0, =(TCR_VALUE)
	msr	tcr_el1, x0

	ldr	x0, =(MAIR_VALUE)
	msr	mair_el1, x0

	bl _start_two
	ldr	x2, =kernel_main

	mov	x0, #SCTLR_MMU_ENABLED
	msr	sctlr_el1, x0	// BOOM! we are on virtual after this.


	br 	x2

	// Given a virt addr and the PGD, set the PGD entry, allocate one PUD and one PMD.
	//		link PGD -> PUD and PUD -> PMD
	// @tbl: a register pointing to PGD
	// @virt: the virtual address that we are currently mapping
	// @tmp1/2: temporary registers to use; contents will be clobbered
	.macro	create_pgd_entry, tbl, virt, tmp1, tmp2
	create_table_entry \tbl, \virt, PGD_SHIFT, \tmp1, \tmp2  // set a PGD entry
	// @tbl now points to the newly created PUD table
	create_table_entry \tbl, \virt, PUD_SHIFT, \tmp1, \tmp2		// set a PUD entry
	// @tbl now points to the newly created PMD table
	.endm

	// Allocating a new page table (either PGD or PUD) for the kernel's initial page tables
	// All the initial page tables are located in one continuous memory region
	//
	// @tbl: a register pointing to the last pgtable in a memory region, from which pgtables
	//			are allocated sequentially
	// @virt: the virtual address that we are currently mapping
	// @shift: 39 in case of PGD and 30 in case of PUD
	// 		   apply to the virtual address in order to extract current table index.
	// @tmp1/2: temporary registers to use; contents will be clobbered
	.macro	create_table_entry, tbl, virt, shift, tmp1, tmp2
	lsr	\tmp1, \virt, #\shift
	and	\tmp1, \tmp1, #PTRS_PER_TABLE - 1		// tmp1: table index
	add	\tmp2, \tbl, #PAGE_SIZE					// tmp2: addr of a next level pgtable (PUD or PMD).
	orr	\tmp2, \tmp2, #MM_TYPE_PAGE_TABLE		// tmp2: make a table descriptor. set bits[0:1] to 1.
	str	\tmp2, [\tbl, \tmp1, lsl #3]			// store descriptor (tmp2) to the current pgtable at index (tmp1)
	add	\tbl, \tbl, #PAGE_SIZE					// point @tbl to the newly create next level pgtable. programming ease
	.endm

	// Populating entries in a PMD table for a given virt addr range
	// @tbl: a reg pointing to the PMD table
	// @phys: the start of the physical region to be mapped
	// @start/@end: virtual address of the first/last section to be mapped
	// @flags: to be copied into lower attributes of the block descriptor
	// @tmp1: temporary register to use; contents will be clobbered
	.macro	create_block_map, tbl, phys, start, end, flags, tmp1
	lsr	\start, \start, #SECTION_SHIFT
	and	\start, \start, #PTRS_PER_TABLE - 1			// start index in the PMD
	lsr	\end, \end, #SECTION_SHIFT
	and	\end, \end, #PTRS_PER_TABLE - 1				// end index in the PMD
	lsr	\phys, \phys, #SECTION_SHIFT				// assmble a table entry
	mov	\tmp1, #\flags
	orr	\phys, \tmp1, \phys, lsl #SECTION_SHIFT			// phys: the table entry value
9999:	str	\phys, [\tbl, \start, lsl #3]				// store the entry in PMD
	add	\start, \start, #1								// @start: index of next PMD entry
	add	\phys, \phys, #SECTION_SIZE						// update the table entry value
	cmp	\start, \end
	b.ls	9999b
	.endm

#ifdef USE_QEMU
__create_idmap:
	mov	x29, x30

	adrp	x0, idmap_dir
	mov	x1, #PG_DIR_SIZE
	bl	memzero

	adrp	x0, idmap_dir
	mov	x1, xzr
	create_pgd_entry	x0, x1, x2, x3

	mov	x1, xzr
	mov	x2, xzr
	ldr	x3, =(PHYS_MEMORY_SIZE)
	create_block_map x0, x1, x2, x3, MMU_FLAGS, x4

	mov	x30, x29
	ret
#endif

__create_page_tables:
    mov     x29, x30

    // clear the mem region backing pgtables
    adrp    x0, pg_dir
    mov     x1, #PG_DIR_SIZE
    bl      memzero

    adrp    x0, pg_dir
    mov     x1, #VA_START
    create_pgd_entry x0, x1, x2, x3     // x0 = PMD0
    mov     x20, x0                     // PMD0


    // x10 = adresse du PUD (selon ta convention: pg_dir + PAGE_SIZE)
    adrp    x10, pg_dir
    add     x10, x10, #PAGE_SIZE        // PUD

    // x21 = adresse du 2e PMD (choisir une page libre)
    adrp    x21, pg_dir
    add     x21, x21, #(3 * PAGE_SIZE)  // PMD1 (pg_dir + 3 pages)

    // Lier PUD[1] -> PMD1
    // (entrée 1 => offset 8, car 8 bytes par entrée)
    orr     x12, x21, #MM_TYPE_PAGE_TABLE
    str     x12, [x10, #8]


    mov     x0, x20
    mov     x1, xzr
    mov     x2, #VA_START
    ldr     x3, =(VA_START + DEVICE_BASE - SECTION_SIZE)
    create_block_map x0, x1, x2, x3, MMU_FLAGS, x4

    mov     x0, x20
    mov     x1, #DEVICE_BASE
    ldr     x2, =(VA_START + DEVICE_BASE)
    ldr     x3, =(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE)
    create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4


    mov     x0, x21
    ldr     x1, =0x40000000
    ldr     x2, =(VA_START + 0x40000000)
    ldr     x3, =(VA_START + 0x40000000)     // 1 bloc de 2MiB (end inclus dans ta macro)
    create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

    mov     x30, x29
    ret

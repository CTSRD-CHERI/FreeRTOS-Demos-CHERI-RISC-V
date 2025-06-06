/*
    FreeRTOS V8.2.3 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution and was contributed
    to the project by Technolution B.V. (www.technolution.nl,
    freertos-riscv@technolution.eu) under the terms of the FreeRTOS
    contributors license.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry''s de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#ifdef NETBOOT
.extern xDtbAddr
#endif

#if __CHERI_PURE_CAPABILITY__
.extern xISRStackTop
.extern pvAlmightyDataCap
.extern pvAlmightyCodeCap

# The CPU starts in hybrid, and the boot initialisation has to be in hybrid.
.option nocapmode
#endif /* __CHERI_PURE_CAPABILITY__ */

/* Startup code */
.section    .boot.text, "wax", @progbits
.align 3
.globl boot
.type boot, @function
boot:
    csrw mcycle, x0
#if __riscv_xlen == 32
    csrw mcycleh, x0
#endif

    li t6, 0x1800
    csrw mstatus, t6

#ifdef __CHERI_PURE_CAPABILITY__
    # W^X for PCC and DDC
    cspecialrw      ct0, ddc, cnull
    li              t1, (1<<1)
    not             t1, t1
    candperm        ct0, ct0, t1
    cspecialrw      cnull, ddc, ct0

    cspecialrw      ct0, pcc, cnull
    li              t1, ((1<<3) | (1<<5))
    not             t1, t1
    candperm        ct0, ct0, t1
    la              t2, _mstart
    csetoffset      ct0, ct0, t2
#ifdef __riscv_xcheri_mode_dependent_jumps
    jr.cap          ct0
#else
    cjr             ct0
#endif

#else
#ifdef NETBOOT
#if __riscv_xlen == 64
    sd              a1, xDtbAddr, t6
#else
    sw              a1, xDtbAddr, t6
#endif
#endif
#endif /* __CHERI_PURE_CAPABILITY__ */
    j _mstart

_mstart:
    li	x1, 0
    li	x2, 0
    li	x3, 0
    li	x4, 0
    li	x5, 0
    li	x6, 0
    li	x7, 0
    li	x8, 0
    li	x9, 0
    li	x10, 0
    li	x11, 0
    li	x12, 0
    li	x13, 0
    li	x14, 0
    li	x15, 0
    li	x16, 0
    li	x17, 0
    li	x18, 0
    li	x19, 0
    li	x20, 0
    li	x21, 0
    li	x22, 0
    li	x23, 0
    li	x24, 0
    li	x25, 0
    li	x26, 0
    li	x27, 0
    li	x28, 0
    li	x29, 0
    li	x30, 0
    li	x31, 0

    /* initialize global pointer */
    la	gp, _gp

init_bss:
#ifndef IMPLICIT_MEM_0
    /* init bss section */
    la	a0, __bss_start
    la	a1, __bss_end /* section end is actually the start of the next section */
    li	a2, 0x0
    jal	fill_block
#endif

init_sbss:
#ifndef IMPLICIT_MEM_0
    /* init sbss section */
    la	a0, __sbss_start
    la	a1, __sbss_end /* section end is actually the start of the next section */
    li	a2, 0x0
    jal	fill_block
#endif

write_stack_pattern:
    /* init bss section */
    la	a0, _stack_end  /* note the stack grows from top to bottom */
    la	a1, __stack   /* section end is actually the start of the next section */
    li	a2, 0xABABABAB
    jal	fill_block

init_stack:
#if __CHERI_PURE_CAPABILITY__
    # From now on, do purecap ABI but in hybrid mode.
    la              sp, _stack_end
    cspecialr       ct2, ddc
    csetaddr        csp, ct2, sp
    la              t2, _stack
    sub             t2, t2, sp # length of the stack
    csetbounds      csp, csp, t2
    cincoffset      csp, csp, t2

    # The same stack is later used by the kernel.
    la              t0, xISRStackTop
    sc              csp, 0(t0)

    cspecialrw      ct0, ddc, cnull # Get DDC.
    la              t1, pvAlmightyDataCap
    sc              ct0, 0(t1)

    cspecialrw      ct0, pcc, cnull # Get PCC.
    li              t1, 1
    csetflags       ct0, ct0, t1
    la              t1, pvAlmightyCodeCap
    sc              ct0, 0(t1)

    # Initialise the captable.
    # ct0 is still PCC
    la              t1, _start_purecap
    csetoffset      ct0, ct0, t1

#ifdef __riscv_xcheri_mode_dependent_jumps
    jalr.cap        cra, ct0
#else
    cjalr           cra, ct0
#endif

    # From now on, the DDC is always null.
    cmove           ct0, cnull
    cspecialrw      cnull, ddc, ct0 # Write DDC.

    # Set up mtcc and jump to main in purecap mode.
    cspecialrw      ct0, pcc, cnull # Get PCC.

    li              t1, 1
    csetflags       ct0, ct0, t1     # set cap mode on

    la              t1, freertos_risc_v_trap_handler
    csetoffset      ct0, ct0, t1
    cspecialrw      cnull, mtcc, ct0 # Write MTCC.

    la              t1, demo_main
    csetoffset      ct0, ct0, t1
#ifdef __riscv_xcheri_mode_dependent_jumps
    jalr.cap        cra, ct0 # jump to main
#else
    cjalr           cra, ct0 # jump to main
#endif
#else

    la t0, freertos_risc_v_trap_handler
    csrw mtvec, t0

    /* set stack pointer */
    la	sp, _stack

	la	t0, demo_main
	jr	t0
#endif /* __CHERI_PURE_CAPABILITY__ */

/* Fills memory blocks */
fill_block:
    bgeu	a0, a1, fb_end
    sw		a2, 0(a0)
    addi	a0, a0, 4
    j		fill_block
fb_end:
    ret
.size boot, . - boot

#ifdef __CHERI_PURE_CAPABILITY__
.option capmode
#endif /* __CHERI_PURE_CAPABILITY__ */

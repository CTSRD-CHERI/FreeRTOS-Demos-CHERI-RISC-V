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
     *    is the industry's de facto standard.                               *
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

OUTPUT_ARCH( "riscv" )

/*
_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x4000;
_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0x4000;
*/

_STACK_SIZE = 0x4000;
_HEAP_SIZE = 0x4000;

/*****************************************************************************
 * Define memory layout
 ****************************************************************************/
MEMORY {
    SRAM : ORIGIN = configSRAM_START, LENGTH = configSRAM_SIZE
    FLASH : ORIGIN = configFLASH_START, LENGTH = configFLASH_SIZE

    fastmem : ORIGIN = configFAST_MEM_START, LENGTH = configFAST_MEM_SIZE
    slowmem : ORIGIN = configSLOW_MEM_START, LENGTH = configSLOW_MEM_SIZE
	uncached : ORIGIN = UNCACHED_MEM_START, LENGTH = 0x0010000
}

/* Specify the default entry point to the program */

ENTRY(boot)

/*****************************************************************************
 * Define the sections, and where they are mapped in memory
 ****************************************************************************/
SECTIONS {

	.text.fast : ALIGN(8) {
        __FLASH_segment_start__ = ABSOLUTE(.);
		*(.boot.text);
	} > FLASH

    priviliged_functions : ALIGN(8) {
      __privileged_functions_start__ = ABSOLUTE(.);
      *(privileged_functions)
      __privileged_functions_end__ = ABSOLUTE(.);
    } > FLASH

	.init : {
	   KEEP (*(.init))
	} > FLASH

    .fini : {
       KEEP (*(.fini))
    } > FLASH

	.text : ALIGN(8) {

      __syscalls_flash_start__ = ABSOLUTE(.);
      *(freertos_system_calls)
      __syscalls_flash_end__ = ABSOLUTE(.);

      __unprivileged_flash_start__ = ABSOLUTE(.);
	  *(.text.fast);
	  *(.text);
	  *(.text.*);
	} > FLASH

    .rodata : {
       __rodata_start = .;
       *(.rodata)
       *(.rodata.*)
       *(.rom)
       *(.gnu.linkonce.r.*)
      __unprivileged_flash_end__ = ABSOLUTE(.);
       __rodata_end = .;
	} > FLASH

    .ramdisk (NOLOAD) :  ALIGN(16) {
      *(.ramdisk)
    } > FLASH

    __cap_relocs :  ALIGN(16) {
      *(__cap_relocs)

      __FLASH_segment_end__ = ABSOLUTE(.);
    } > FLASH

    .captable : ALIGN(16) {
       __SRAM_segment_start__ = ABSOLUTE(.);
       __privileged_data_start__ = ABSOLUTE(.);
      *(.captable)
    } > SRAM

	.data : {
	   __data_start = .;
       *(privileged_data)
       __privileged_data_end__ = ABSOLUTE(.);

       __unprivileged_sram_start__ = ABSOLUTE(.);

	   *(.data)
	   *(.data.*)
	   *(.gnu.linkonce.d.*)
	   __data_end = .;
	} > SRAM

    .sdata : {
        _gp = . + 0x800;
        *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
     }	> SRAM

	.heap_kernel (NOLOAD):  ALIGN(16) {
       *(privileged_heap)
	} > SRAM

    .stack (NOLOAD) :  ALIGN(16) {
       _stack_end = .;
       . += _STACK_SIZE;
       _stack = .;
       __stack = _stack;
       __freertos_irq_stack_top = .;
    } > SRAM

	.htif (NOLOAD) : {
	   *(.htif)
	} > SRAM

    .bss (NOLOAD) :  ALIGN(16) {
       __bss_start = .;
       *(.bss)
       *(.bss.*)
       *(.gnu.linkonce.b.*)
       *(COMMON)
       . = ALIGN(16);
       __bss_end = .;
    } > SRAM

    .sbss (NOLOAD) : {
       __sbss_start = .;
       *(.sbss)
       *(.sbss.*)
       *(.gnu.linkonce.sb.*)
       __sbss_end = .;
      __unprivileged_sram_end__ = ABSOLUTE(.);
    } > SRAM

    __SRAM_segment_end__ = ABSOLUTE(.);

    .uncached (NOLOAD) : {
       *(.uncached)
    } > uncached

    _end = .;
}

/*
 * FreeRTOS Kernel V10.1.1
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/**
 * This is docs for main
 */

/* FreeRTOS kernel includes. */

#include <time.h>

#include <FreeRTOS.h>
#include <task.h>

#ifdef mainCONFIG_USE_DYNAMIC_LOADER
/* FreeRTOS-libdl includes to dynamically load and link objects */
#include <dlfcn.h>
#include <rtl/rtl-trace.h>
#include <rtl/rtl-archive.h>

/* FreeRTOS+FAT includes. */
#include "ff_headers.h"
#include "ff_stdio.h"
#include "ff_ramdisk.h"
#endif

/* Bsp includes. */
#include "bsp.h"

#ifdef CONFIG_ENABLE_CHERI
#include <cheri_init_globals.h>
#include <cheri/cheri-utility.h>

void
_start_purecap(void) {
  cheri_init_globals_3(__builtin_cheri_global_data_get(),
      __builtin_cheri_program_counter_get(),
      __builtin_cheri_global_data_get());
}
#endif /* CONFIG_ENABLE_CHERI */

/******************************************************************************
 */
#if mainDEMO_TYPE == 1
#pragma message "Demo type 1: Basic Blinky"
extern void main_blinky(void);
#elif mainDEMO_TYPE == 3
#pragma message "Demo type 3: Tests"
extern void main_tests(void);
#elif mainDEMO_TYPE == 4
#pragma message "Demo type 4: Compartment Tests"
extern void main_compartment_test(void);
#elif mainDEMO_TYPE == 5
#pragma message "Demo type 5: TCP/IP peekpoke Test"
extern void main_peekpoke(void);
#elif mainDEMO_TYPE == 6
#pragma message "Demo type : UDP/TCP/IP-based echo, CLI, HTTP, FTP and TFTP servers"
extern void main_servers(void);
#elif mainDEMO_TYPE == 42
#pragma message "Demo type 42: Modbus"
extern void main_modbus(void);
#else
#ifdef configPROG_ENTRY
extern void configPROG_ENTRY(void);
#else
#error "Unsupported demo type"
#endif
#endif

/* Prototypes for the standard FreeRTOS callback/hook functions implemented
within this file.  See https://www.freertos.org/a00016.html */
void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationTickHook(void);

void vToggleLED(void);

#if __riscv_xlen == 64
#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })
#endif

/**
 * Capture the current 64-bit cycle count.
 */
uint64_t get_cycle_count(void)
{
#if __riscv_xlen == 64
	return read_csr(mcycle);
#else
	uint32_t cycle_lo, cycle_hi;
	asm volatile(
		"%=:\n\t"
		"csrr %1, mcycleh\n\t"
		"csrr %0, mcycle\n\t"
		"csrr t1, mcycleh\n\t"
		"bne  %1, t1, %=b"
		: "=r"(cycle_lo), "=r"(cycle_hi)
		: // No inputs.
		: "t1");
	return (((uint64_t)cycle_hi) << 32) | (uint64_t)cycle_lo;
#endif
}

/**
 * Use `mcycle` counter to get usec resolution.
 * On RV32 only, reads of the mcycle CSR return the low 32 bits,
 * while reads of the mcycleh CSR return bits 63–32 of the corresponding
 * counter.
 * We convert the 64-bit read into usec. The counter overflows in roughly an hour
 * and 20 minutes. Probably not a big issue though.
 * At 50HMz clock rate, 1 us = 50 ticks
 */
uint32_t port_get_current_mtime(void)
{
	return (uint32_t)(get_cycle_count() / (configCPU_CLOCK_HZ / 1000000));
}
/*-----------------------------------------------------------*/

#if mainCONFIG_USE_DYNAMIC_LOADER

#ifndef configPROG_ENTRY
#error "configPROG_ENTRY must be defined as the entry function for the application \
       "to which the dynamic loader jumps to"
#endif

/* The number and size of sectors that will make up the RAM disk.  The RAM disk
is huge to allow some verbose FTP tests. */
#define mainRAM_DISK_SECTOR_SIZE    512UL /* Currently fixed! */
#define mainRAM_DISK_SECTORS        ( ( 5UL * 1024UL * 1024UL ) / mainRAM_DISK_SECTOR_SIZE ) /* 5M bytes. */
#define mainIO_MANAGER_CACHE_SIZE   ( 15UL * mainRAM_DISK_SECTOR_SIZE )

void vFatEmbedLibFiles( void );

static void prvLoader(void) {

typedef void (*prog_entry_t)( void );
static uint8_t ucRAMDisk[ mainRAM_DISK_SECTORS * mainRAM_DISK_SECTOR_SIZE ];
FF_Disk_t *pxDisk;
prog_entry_t entry = NULL;

    /* Create the RAM disk. */
    pxDisk = FF_RAMDiskInit( mainRAM_DISK_NAME, ucRAMDisk, mainRAM_DISK_SECTORS, mainIO_MANAGER_CACHE_SIZE );
    configASSERT( pxDisk );

    /* Print out information on the disk. */
    FF_RAMDiskShowPartition( pxDisk );

    /* Embed the libs in the file systems*/
    vFatEmbedLibFiles();

    rtems_rtl_trace_set_mask ( RTEMS_RTL_TRACE_UNRESOLVED);

    // TODO: Make this generic to load the main object of any program
    void *obj = dlopen("/lib/libmain_servers.a:main_servers.c.1.o", RTLD_GLOBAL | RTLD_NOW);

    if (obj == NULL) {
      printf("Failed to dynamically load the app and/or libs\n");
      _exit(-1);
    }

     #define str(s) #s
     #define xstr(s) str(s)

    printf("Searching loaded objects for %s\n", xstr(configPROG_ENTRY));
    entry = (prog_entry_t) dlsym(obj, xstr(configPROG_ENTRY));

    if (entry == NULL) {
      printf("Failed to to find the specified prog entry: %s\n", xstr(configPROG_ENTRY));
      _exit(-1);
    } else {
      printf("Found entry %s @ %p\n", xstr(configPROG_ENTRY), entry);
    }

    printf("Jumping to the app entry: %s\n", xstr(configPROG_ENTRY));

#ifdef __CHERI_PURE_CAPABILITY__
  void* data_cap = NULL;
  int ret = dlinfo(obj, RTLD_DI_CHERI_CAPTABLE, &data_cap);
  printf("CCalling code -> "); cheri_print_cap(entry);
  printf("CCalling captable -> "); cheri_print_cap(data_cap);
  asm volatile(".balign 4\nccall %0, %1":: "C"(entry), "C"(data_cap):);
#else
  vTaskSuspendAll();
  entry();
#endif
  xTaskResumeAll();
  while(1);
}
#endif
/*-----------------------------------------------------------*/

int demo_main(void) {

  prvSetupHardware();

  /* The mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting is described at the top
  	of this file. */
#if mainDEMO_TYPE == 1
  {
    main_blinky();
  }
#elif mainDEMO_TYPE == 3
  {
    main_tests();
  }
#elif mainDEMO_TYPE == 4
  {
    main_compartment_test();
  }
#elif mainDEMO_TYPE == 5
  {
    main_peekpoke();
  }
#elif mainDEMO_TYPE == 6
  {
    main_servers();
  }
#elif mainDEMO_TYPE == 42
  /* run the main_modbus demo */
  {
    main_modbus();
  }
#else
#ifdef configPROG_ENTRY
#ifdef mainCONFIG_USE_DYNAMIC_LOADER
  /* Create a task that dynamically loads libs and apps from the file system */
  xTaskCreate(prvLoader,
              "loader",
              configMINIMAL_STACK_SIZE * 2U,
              NULL,
              tskIDLE_PRIORITY,
              NULL);
#else
  configPROG_ENTRY();
#endif
#else
#error "Unsupported Demo"
#endif
#endif
	vTaskStartScheduler();
	for (;;);
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
time_t FreeRTOS_time( time_t *pxTime )
{
time_t xReturn;

    xReturn = time( &xReturn );

    if( pxTime != NULL )
    {
        *pxTime = xReturn;
    }

    return xReturn;
}

// TODO: toggle a real LED at some point
void vToggleLED(void) {
  static uint32_t ulLEDState = 0;

  // 	GPIO_set_outputs( &g_gpio_out, ulLEDState );
  ulLEDState = !ulLEDState;
}
// /*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void) {
  /* vApplicationMallocFailedHook() will only be called if
  configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
  function that will get called if a call to pvPortMalloc() fails.
  pvPortMalloc() is called internally by the kernel whenever a task, queue,
  timer or semaphore is created.  It is also called by various parts of the
  demo application.  If heap_1.c or heap_2.c are used, then the size of the
  heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
  FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
  to query the size of free heap space that remains (although it does not
  provide information on how the remaining heap might be fragmented). */
  taskDISABLE_INTERRUPTS();
  __asm volatile("ebreak");
  for (;;)
    ;
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void) {
  /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
  to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
  task.  It is essential that code added to this hook function never attempts
  to block in any way (for example, call xQueueReceive() with a block time
  specified, or call vTaskDelay()).  If the application makes use of the
  vTaskDelete() API function (as this demo application does) then it is also
  important that vApplicationIdleHook() is permitted to return to its calling
  function, because it is the responsibility of the idle task to clean up
  memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
  (void)pcTaskName;
  (void)pxTask;

  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
  function is called if a stack overflow is detected. */
  taskDISABLE_INTERRUPTS();
  __asm volatile("ebreak");
  for (;;)
    ;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void) {
  /* The tests in the full demo expect some interaction with interrupts. */
#if (mainDEMO_TYPE == 2)
  {
    extern void vFullDemoTickHook(void);
    vFullDemoTickHook();
  }
#endif
}

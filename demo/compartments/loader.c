/*
 * FreeRTOS Kernel V10.1.1
 * Copyright (C) 2020 Hesham Almatary <hesham.almatary@cl.cam.ac.uk>
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


/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <sys/exec_elf.h>
#include <rtl/rtl-trace.h>
#include <rtl/rtl-freertos-compartments.h>

#include <inttypes.h>
#if __riscv_xlen == 32
#define PRINT_REG "0x%08" PRIx32
typedef Elf32_Sym Elf_Sym;
#elif __riscv_xlen == 64
#define PRINT_REG "0x%016" PRIx64
typedef Elf64_Sym Elf_Sym;
#endif

#define CHERI_ELF_PT_TYPE_MASK 0xfc00u
#define CHERI_ELF_PT_ID_MASK ~(CHERI_ELF_PT_TYPE_MASK)
#define CHERI_ELF_PT_COMPARTMENT 0xf800
#define CHERI_ELF_PT_COMPSYMTAB  0xfc00

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheric.h>
extern void *pvAlmightyDataCap;
extern void *pvAlmightyCodeCap;
#endif /* __CHERI_PURE_CAPABILITY__ */

#define TEST_TRACE 1
#if TEST_TRACE
 #define DEBUG_TRACE (RTEMS_RTL_TRACE_DETAIL | \
                      RTEMS_RTL_TRACE_WARNING | \
                      RTEMS_RTL_TRACE_LOAD | \
                      RTEMS_RTL_TRACE_UNLOAD | \
                      RTEMS_RTL_TRACE_SYMBOL | \
                      RTEMS_RTL_TRACE_RELOC | \
                      RTEMS_RTL_TRACE_ALLOCATOR | \
                      RTEMS_RTL_TRACE_UNRESOLVED | \
                      RTEMS_RTL_TRACE_ARCHIVES | \
                      RTEMS_RTL_TRACE_CACHE | \
                      RTEMS_RTL_TRACE_DEPENDENCY)
 #define DL_DEBUG_TRACE DEBUG_TRACE /* RTEMS_RTL_TRACE_ALL */
 #define DL_RTL_CMDS    1
#else
 #define DL_DEBUG_TRACE 0
 #define DL_RTL_CMDS    0
#endif

void vCompartmentsLoad(void);

static void vTaskCompartment(void *pvParameters);
static UBaseType_t cheri_exception_handler();
static UBaseType_t default_exception_handler(uintptr_t *exception_frame);
static void vSymEntryPrint( Elf_Sym *entry );
static void elf_manip( void );

#ifdef __CHERI_PURE_CAPABILITY__
static void *cheri_create_cap(ptraddr_t base, size_t size) {
    void *return_cap;
    return_cap = cheri_setoffset( pvAlmightyCodeCap, base );
    return_cap = cheri_csetbounds( return_cap, size );
    return return_cap;
}
#endif

extern char _headers_end[];
char *size_headers = &_headers_end;

static void vCompartmentsElfPrint( void )
{
Elf64_Phdr *phdr = (void *) 0x80000040;
#ifdef __CHERI_PURE_CAPABILITY__
    phdr = cheri_create_cap( (ptraddr_t) phdr, (size_t) _headers_end);
#endif

  //for(int i = 0; i < (size_t) (_headers_end) / sizeof(Elf64_Phdr); i++) {
  for(int i = 0; i < (size_t) (size_headers) / sizeof(Elf64_Phdr); i++) {
    if((phdr[i].p_flags & CHERI_ELF_PT_TYPE_MASK) == CHERI_ELF_PT_COMPARTMENT) {

      UBaseType_t comp_id = (phdr[i].p_flags & CHERI_ELF_PT_ID_MASK);
      printf("comp id = %u\n", comp_id);
#ifdef __CHERI_PURE_CAPABILITY__
      void *cap = cheri_create_cap((ptraddr_t) phdr[i].p_paddr, phdr[i].p_memsz);
#else
      void *cap = (void *) phdr[i].p_paddr;
#endif

      printf("- Compartment #%u with name: %s starting at 0x%lx of size %u\n",
             (phdr[i].p_flags & CHERI_ELF_PT_ID_MASK),
             comp_strtab[comp_id],
             phdr[i].p_paddr,
             phdr[i].p_memsz);

      comp_list[comp_id].cap = cap;
      comp_list[comp_id].size =  phdr[i].p_memsz;
      comp_list[comp_id].name = comp_strtab[comp_id];
    }
  }
}

void vCompartmentsLoad(void) {
  void *obj_handle;

  printf("Starting Compartments Loading\n");

  vCompartmentsElfPrint();

#if DL_DEBUG_TRACE
  rtems_rtl_trace_set_mask (DL_DEBUG_TRACE);
#endif

  printf("load: comp1.o\n");

  obj_handle = dlopen ("comp1.o", RTLD_NOW | RTLD_GLOBAL);
  if (!obj_handle)
  {
    printf("dlopen failed: %s\n", dlerror());
  }

}
/*-----------------------------------------------------------*/

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
#include <elf.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

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

typedef struct compartment {
  void        *cap;
  uintcap_t   *cap_list;
  // TCB?
  // Symtab?
} compartment_t;

compartment_t comp_list[configCOMPARTMENTS_NUM];

void vCompartmentsLoad(void);

static void vTaskCompartment(void *pvParameters);
static UBaseType_t cheri_exception_handler();
static UBaseType_t default_exception_handler(uintptr_t *exception_frame);
static void vSymEntryPrint( Elf_Sym *entry );
static void elf_manip( void );

static void *cheri_create_cap(ptraddr_t base, size_t size) {
    void *return_cap;
    return_cap = cheri_setoffset( pvAlmightyCodeCap, base );
    return_cap = cheri_csetbounds( return_cap, size );
    return return_cap;
}

static void vCompartmentsElfPrint( void )
{
Elf64_Phdr *phdr = (void *) 0x80000040;
extern char _headers_end[];
size_t headers_size =  (ptraddr_t) _headers_end - 0x080000000;
    phdr = cheri_setoffset( pvAlmightyDataCap, ( ptraddr_t ) phdr );
    phdr = cheri_csetbounds( ( void * ) phdr, (size_t) _headers_end );

  for(int i = 0; i < (size_t) (_headers_end) / sizeof(Elf64_Phdr); i++) {
    if((phdr[i].p_flags & CHERI_ELF_PT_TYPE_MASK) == CHERI_ELF_PT_COMPARTMENT) {

      UBaseType_t comp_id = (phdr[i].p_flags & CHERI_ELF_PT_ID_MASK);
      printf("comp id = %u\n", comp_id);
      void *cap = cheri_create_cap((ptraddr_t) phdr[i].p_paddr, phdr[i].p_memsz);

      printf("- Compartment #%u starting at 0x%lx of size %u\n",
             (phdr[i].p_flags & CHERI_ELF_PT_ID_MASK),
             phdr[i].p_paddr,
             phdr[i].p_memsz);

      comp_list[comp_id].cap = cap;
    }
  }
}

void vCompartmentsLoad(void) {
  printf("Starting Compartments Loading\n");

  vCompartmentsElfPrint();
}
/*-----------------------------------------------------------*/

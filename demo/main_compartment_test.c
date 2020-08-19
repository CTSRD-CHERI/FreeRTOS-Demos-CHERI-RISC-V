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
#include <cheric.h>
extern void *pvAlmightyDataCap;
extern void *pvAlmightyCodeCap;

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

static void vElfGetCompartments( void )
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
    } else if ((phdr[i].p_flags & CHERI_ELF_PT_TYPE_MASK) == CHERI_ELF_PT_COMPSYMTAB) {
      UBaseType_t comp_id = (phdr[i].p_flags & CHERI_ELF_PT_ID_MASK);
      void *cap_symtab = cheri_create_cap((ptraddr_t) phdr[i].p_paddr, phdr[i].p_memsz * sizeof(Elf_Sym));

      for(int i = 0; i < phdr[i].p_memsz; i++) {
        //printf("- SymTab Compartment #%u\n", comp_id);
        //vSymEntryPrint(((Elf_Sym *) cap_symtab) + i);
      }
    }
  }
}

/*-----------------------------------------------------------*/
static void vElfHeaderPrint( void )
{
extern char  __strtab_start[];
Elf64_Ehdr *phdr = (void *) 0x80000000;
	phdr = cheri_setoffset( pvAlmightyDataCap, ( ptraddr_t ) phdr );
	phdr = cheri_csetbounds( ( void * ) phdr, sizeof( Elf64_Ehdr ) );

  printf("ehdr->Ie_ident = %s\n", &phdr->e_ident[1]);
  printf("ehdr->e_type = %llu\t e_phoff = %llx \n", phdr->e_type, phdr->e_phoff);
  printf("ehdr->phoff = %x\n", phdr->e_phoff);
}
/*-----------------------------------------------------------*/

static void vElfProgramHeaderPrint( void )
{
extern char  __strtab_start[];
Elf64_Phdr *phdr = (void *) 0x80000040;
extern char _headers_end[];
size_t headers_size =  (ptraddr_t) _headers_end - 0x080000000;
printf("_headers_end = %lx\n", (ptraddr_t) _headers_end);
printf("Headers Size = %lu\n", headers_size);
// while(1);
	phdr = cheri_setoffset( pvAlmightyDataCap, ( ptraddr_t ) phdr );
	phdr = cheri_csetbounds( ( void * ) phdr, (size_t) _headers_end );

  for(int i = 0; i < (size_t) (_headers_end) / sizeof(Elf64_Phdr); i++) {
    printf("phdr[%u]->p_type = %lu\t paddr = %llx \t p_flags = %lu\n", i, phdr[i].p_type, phdr[i].p_paddr, phdr[i].p_flags);
  }
}
/*-----------------------------------------------------------*/

static void vSymEntryPrint( Elf_Sym *entry )
{
extern char  __strtab_start[];
	printf("st_name[]: %s\t", &__strtab_start[entry->st_name]);
	printf("st_value: %lx\t", entry->st_value);
	printf("st_size: %lx\t", entry->st_size);
	printf("st_shndx: %lu\n", entry->st_shndx);
}
/*-----------------------------------------------------------*/

static void elf_manip( void )
{
extern Elf_Sym  __symtab_start[];
extern Elf_Sym  __symtab_end[];
extern char  __strtab_start[];
extern char  __strtab_end[];
extern char  __dynsym_start[];
extern char  __dynsym_end[];
//extern Elf_Sym  __symtab_shndx_start[];
//extern Elf_Sym  __symtab_shndx_end[];
uint32_t entries_num =  ((ptraddr_t) __symtab_end - (ptraddr_t) __symtab_start) / sizeof(Elf_Sym);
uint32_t dynentries_num =  (__dynsym_end - __dynsym_start) / sizeof(Elf_Sym);
//uint32_t section_ndex_num =  ((ptraddr_t)__symtab_shndx_end - (ptraddr_t)__symtab_shndx_start) / sizeof(Elf_Sym);

//extern Elf_Sym str_table[] = (Elf_Sym *) __symtab_start;
	printf("sizeof(Elf_Sym) = %u\n", sizeof(Elf_Sym));
	printf("__symtab_start = %p\n", __symtab_start);
	printf("__symtab_end = %p\n", __symtab_end);
	printf("__dynsym_start = %p\n", __dynsym_start);
	printf("__dynsym_end = %p\n", __dynsym_end);
	printf("__strab_start = %p\n", __strtab_start);
	printf("__strtab_end = %p\n", __strtab_end);
	//printf("__symtab_shndx_start = %p\n", __symtab_shndx_start);
	//printf("__symtab_shndx_end = %p\n", __symtab_shndx_end);
	printf("entries_num = %u\n", entries_num);
	printf("dynentries_num = %u\n", dynentries_num);
	//printf("section_ndex_num = %u\n", section_ndex_num);

	for(int i = 0; i < entries_num; i++)
	{
    //printf("entry = %u\n", i);
    if ((__symtab_start[i].st_info & 0xf) == STT_SECTION)
		  vSymEntryPrint(&__symtab_start[i]);
	}

	//for(int i = 0; i < section_ndex_num; i++)
	//{
    //printf("symtab_shndx = %u\n", i);
		//vSymEntryPrint(&__symtab_shndx_start[i]);
	//}

  //vElfHeaderPrint();

  printf("CHERI Compartments Num = %u\n", configCOMPARTMENTS_NUM);

  vElfProgramHeaderPrint();

  vElfGetCompartments();

	printf("Done with symbols\n");
}
/*-----------------------------------------------------------*/

void main_compartment_test(void) {

    xTaskCreate(vTaskCompartment,			 /* The function that implements the task. */
                "Compartment Task",					 /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                configMINIMAL_STACK_SIZE * 2U,   /* The size of the stack to allocate to the task. */
                NULL,							 /* The parameter passed to the task - not used in this case. */
                1, /* The priority assigned to the task. */
                NULL);							 /* The task handle is not required, so NULL is passed. */

    for (int i = 0; i < 64; i++) {
      vPortSetExceptionHandler(i, default_exception_handler);
    }
    /* Setup an exception handler for CHERI */
    vPortSetExceptionHandler(0x1c, cheri_exception_handler);

    /* Print symtable */
    elf_manip();

    vCompartmentsLoad();

    /* Start the tasks and timer running. */
    vTaskStartScheduler();

  /* If all is well, the scheduler will now be running, and the following
  line will never be reached.  If the following line does execute, then
  there was insufficient FreeRTOS heap memory available for the Idle and/or
  timer tasks to be created.  See the memory management section on the
  FreeRTOS web site for more details on the FreeRTOS heap
  http://www.freertos.org/a00111.html. */
  for (;;)
    ;
}
/*-----------------------------------------------------------*/

static void vTaskCompartment(void *pvParameters) {

  printf("Testing Compartment\n");
  _exit(0);
  while(1);
}
/*-----------------------------------------------------------*/


static void cheri_print_cap(void *cap) {
  printf("cap: [v: %" PRIu8 " | f: %" PRIu8 " | sealed: %" PRIu8 " | addr: "
  PRINT_REG \
  " | base: " PRINT_REG " | length: " PRINT_REG " | offset: " PRINT_REG \
  " | perms: " PRINT_REG "] \n",
  (uint8_t) __builtin_cheri_tag_get(cap),
  (uint8_t) __builtin_cheri_flags_get(cap),
  (uint8_t) __builtin_cheri_sealed_get(cap),
  __builtin_cheri_address_get(cap),
  __builtin_cheri_base_get(cap),
  __builtin_cheri_length_get(cap),
  __builtin_cheri_offset_get(cap),
  __builtin_cheri_perms_get(cap)
  );
}

static UBaseType_t cheri_exception_handler(uintptr_t *exception_frame)
{
  cheri_print_cap(*(++exception_frame));
  while(1);
}

static UBaseType_t default_exception_handler(uintptr_t *exception_frame)
{
  size_t cause = 0;
  size_t epc = 0;
  asm volatile ("csrr %0, mcause" : "=r"(cause)::);
  asm volatile ("csrr %0, mepc" : "=r"(epc)::);
  printf("mcause = %u\n", cause);
  printf("mepc = %u\n", epc);
  while(1);
}
/*-----------------------------------------------------------*/

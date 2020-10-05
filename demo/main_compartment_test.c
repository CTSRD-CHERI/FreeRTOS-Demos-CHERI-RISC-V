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
#include <inttypes.h>
#include <stdbool.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#if __riscv_xlen == 32
#define PRINT_REG "0x%08" PRIx32
#elif __riscv_xlen == 64
#define PRINT_REG "0x%016" PRIx64
#endif

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri-utility.h>
#include <rtl/rtl-freertos-compartments.h>
#endif

void vCompartmentsLoad(void);

static void vTaskCompartment(void *pvParameters);
static UBaseType_t cheri_exception_handler();
static UBaseType_t default_exception_handler(uintptr_t *exception_frame);

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

  vCompartmentsLoad();

  _exit(0);
  while(1);
}
/*-----------------------------------------------------------*/

#ifdef __CHERI_PURE_CAPABILITY__

static void ccall_handler(uintptr_t *exception_frame, ptraddr_t mepc) {
  uint32_t *ccall_instruction;
  uint8_t  code_reg_num, data_reg_num;
  uint32_t *mepcc = (uint32_t *) *(exception_frame);
  mepcc -= 1; // portASM has already stepped into the next isntruction;

  ccall_instruction = mepcc;

  // Decode the code/data pair passed to ccall
  code_reg_num = (*ccall_instruction >> 15) & 0x1f;
  data_reg_num = (*ccall_instruction >> 20) & 0x1f;

  // Get the callee CompID (its otype)
  size_t otype = __builtin_cheri_type_get(*(exception_frame + code_reg_num));

  xCOMPARTMENT_RET ret = xTaskRunCompartment(cheri_unseal_cap(*(exception_frame + code_reg_num)),
                    cheri_unseal_cap(*(exception_frame + data_reg_num)),
                    exception_frame + 10,
                    otype);

  // Save the return registers in the context.
  // FIXME: Some checks might be done here to check of the compartment traps and
  // accordingly take some different action rather than just returning
  *(exception_frame + 10) = ret.a0;
  *(exception_frame + 11) = ret.a1;
}

#endif

static UBaseType_t cheri_exception_handler(uintptr_t *exception_frame)
{
#ifdef __CHERI_PURE_CAPABILITY__
  size_t cause = 0;
  size_t epc = 0;
  size_t cheri_cause;

  asm volatile ("csrr %0, mcause" : "=r"(cause)::);
  asm volatile ("csrr %0, mepc" : "=r"(epc)::);

  size_t ccsr = 0;
  asm volatile ("csrr %0, mccsr" : "=r"(ccsr)::);

  uint8_t reg_num = (uint8_t) ((ccsr >> 10) & 0x1f);
  bool is_scr = ((ccsr >> 15) & 0x1);
  cheri_cause = (unsigned) ((ccsr >> 5) & 0x1f);


  // ccall
  if (cheri_cause == 0x19) {
    ccall_handler(exception_frame, epc);
    return 0;
  }

  for(int i = 0; i < 35; i++) {
    printf("x%i ", i); cheri_print_cap(*(exception_frame + i));
  }

  printf("mepc = 0x%lx\n", epc);
  printf("TRAP: CCSR = 0x%lx (cause: %x reg: %u : scr: %u)\n",
               ccsr,
               cheri_cause,
               reg_num, is_scr);
#endif
  while(1);
}

static UBaseType_t default_exception_handler(uintptr_t *exception_frame)
{
  size_t cause = 0;
  size_t epc = 0;
  asm volatile ("csrr %0, mcause" : "=r"(cause)::);
  asm volatile ("csrr %0, mepc" : "=r"(epc)::);
  printf("mcause = %u\n", cause);
  printf("mepc = %llx\n", epc);
  while(1);
}
/*-----------------------------------------------------------*/

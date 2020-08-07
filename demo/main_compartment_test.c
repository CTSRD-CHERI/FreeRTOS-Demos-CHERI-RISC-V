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

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <inttypes.h>
#include <cheric.h>

#if __riscv_xlen == 32
#define PRINT_REG "0x%08" PRIx32
#elif __riscv_xlen == 64
#define PRINT_REG "0x%016" PRIx64
#endif

static void vTaskCompartment(void *pvParameters);
static UBaseType_t cheri_exception_handler();

/*-----------------------------------------------------------*/

void main_compartment_test(void) {

    xTaskCreate(prvvTaskCompartment,			 /* The function that implements the task. */
                "Compartment Task",					 /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                configMINIMAL_STACK_SIZE * 2U,   /* The size of the stack to allocate to the task. */
                NULL,							 /* The parameter passed to the task - not used in this case. */
                1, /* The priority assigned to the task. */
                NULL);							 /* The task handle is not required, so NULL is passed. */

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
/*-----------------------------------------------------------*/

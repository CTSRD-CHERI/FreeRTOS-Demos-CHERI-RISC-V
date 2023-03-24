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

/******************************************************************************
 * This FreeRTOS instance runs the Gups OpenShmem benchmark as a minimal demo
 * of some OpenShmem functions hosted under FreeRTOS.
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* application includes */
#include "SHMEMRandomAccess.c"

/*-----------------------------------------------------------*/

/*
 * Called by main when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1 in
 * main.c.
 */
void main_bgas_demo( void );

/*
 * The tasks as described in the comments at the top of this file.
 */
static void runGupsBench( void * pvParameters );

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;
#define mainQUEUE_LENGTH                   ( 1 )
#define mainQUEUE_RECEIVE_TASK_PRIORITY    ( tskIDLE_PRIORITY + 2 )
/*-----------------------------------------------------------*/

void main_bgas_demo( void )
{
    /* Create the queue. */
    xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( unsigned long ) );

    if( xQueue != NULL )
    {
        /* Start the two tasks as described in the comments at the top of this
         * file. */
        xTaskCreate( runGupsBench,             /* The function that implements the task. */
                     "Rx",                            /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                     configMINIMAL_STACK_SIZE * 2U,   /* The size of the stack to allocate to the task. */
                     NULL,                            /* The parameter passed to the task - not used in this case. */
                     mainQUEUE_RECEIVE_TASK_PRIORITY, /* The priority assigned to the task. */
                     NULL );                          /* The task handle is not required, so NULL is passed. */

        /* Start the scheduler */

        if( xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED )
        {
            vTaskStartScheduler();
        }

        if( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED )
        {
            xTaskResumeAll();
        }
    }
}
/*-----------------------------------------------------------*/

char fake_exe_name[] = "gups_exe";
char *argv[] = {fake_exe_name};

static void runGupsBench( void * pvParameters )
{
    TickType_t xNextWakeTime;
    const unsigned long ulValueToSend = 100UL;
    BaseType_t xReturned;

    unsigned int cnt = 0;

    /* Remove compiler warning about unused parameter. */
    ( void ) pvParameters;

    /* Initialise xNextWakeTime - this only needs to be done once. */
    xNextWakeTime = xTaskGetTickCount();

    printf("Starting GUPS\r\n");

    main_gups(1, argv);
}
/*-----------------------------------------------------------*/

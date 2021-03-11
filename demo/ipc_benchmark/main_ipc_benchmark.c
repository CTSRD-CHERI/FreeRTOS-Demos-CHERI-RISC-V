/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Hesham Almatary
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "logging.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Priorities used by the tasks. */
#define mainTASK_PRIORITY                  ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_RECEIVE_TASK_PRIORITY    ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_SEND_TASK_PRIORITY       ( tskIDLE_PRIORITY + 1 )

/* The maximum number items the queue can hold.  The priority of the receiving
 * task is above the priority of the sending task, so the receiving task will
 * preempt the sending task and remove the queue items each time the sending task
 * writes to the queue.  Therefore the queue will never have more than one item in
 * it at any time, and even with a queue length of 1, the sending task will never
 * find the queue full. */
#define mainQUEUE_LENGTH                   ( 1 )
/*-----------------------------------------------------------*/

typedef struct taskParams
{
    UBaseType_t xBufferSize;
    UBaseType_t xTotalSize;
    QueueHandle_t xQueue;
    TaskHandle_t mainTask;
} IPCTaskParams_t;
/*-----------------------------------------------------------*/

void queueReceiveTask( void * pvParameters );
void queueSendTask( void * pvParameters );
void main_ipc_benchmark( int argc,
                         char ** argv );
/*-----------------------------------------------------------*/

uint64_t xStartTime = 0;
uint64_t xEndTime = 0;
uint64_t xStartInstRet = 0;
uint64_t xEndInstRet = 0;
static TaskHandle_t sendTask = NULL;
static TaskHandle_t recvTask = NULL;
static UBaseType_t xIterations = RUNS;
static IPCTaskParams_t params;
/*-----------------------------------------------------------*/

void main_ipc_benchmark( int argc,
                         char ** argv )
{
    params = ( IPCTaskParams_t ) {
        IPC_BUFFER_SIZE, IPC_TOTAL_SIZE, NULL
    };

    log( "Started main_ipc_benchmark: #%d args\n", argc );
    log( "./main_ipc_benchmark " );

    for( int i = 0; i < argc; i++ )
    {
        log( "%s ", argv[ i ] );

        if( strcmp( argv[ i ], "-b" ) == 0 )
        {
            params.xBufferSize = strtol( argv[ ++i ], NULL, 10 );
            log( "%s ", argv[ i ] );
        }

        if( strcmp( argv[ i ], "-t" ) == 0 )
        {
            params.xTotalSize = strtol( argv[ ++i ], NULL, 10 );
            log( "%s ", argv[ i ] );
        }

        if( strcmp( argv[ i ], "-n" ) == 0 )
        {
            xIterations = strtol( argv[ ++i ], NULL, 10 );
            log( "%s ", argv[ i ] );
        }
    }

    log( "\n" );

    /* Create the queue. */
    params.xQueue = xQueueCreate( mainQUEUE_LENGTH, params.xBufferSize );
    params.mainTask = xTaskGetCurrentTaskHandle();

    /* Raise our priority */
    vTaskPrioritySet( NULL, tskIDLE_PRIORITY + mainTASK_PRIORITY );

    if( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED )
    {
        xTaskResumeAll();
    }

    if( params.xQueue != NULL )
    {
        for( int i = 0; i < xIterations + DISCARD_RUNS; i++ )
        {
            log( "run #%d: ", i );
            xTaskCreate( queueReceiveTask, "RX", configMINIMAL_STACK_SIZE * 2U, &params, mainQUEUE_RECEIVE_TASK_PRIORITY, &recvTask );
            xTaskCreate( queueSendTask, "TX", configMINIMAL_STACK_SIZE * 2U, &params, mainQUEUE_SEND_TASK_PRIORITY, &sendTask );

            /* Wait for the receive task to notify us all is done */
            ulTaskNotifyTake( pdFALSE, portMAX_DELAY );

            /* vTaskStartScheduler is called by main.c after we return */
        }
    }

    log( "Ended main_ipc_benchmark\n" );

    /* There are two cases main_ipc_benchmark can execute in:
     * 1) When called from main.c, in which case we're expected to return so that
     * main.c calls vTaskStartScheduler.
     * 2) When it gets dynamically loaded in which case the scheduler has already
     * been started.
     * In both cases, we need to return (to main.c or to whoever loaded us). */
}
/*-----------------------------------------------------------*/

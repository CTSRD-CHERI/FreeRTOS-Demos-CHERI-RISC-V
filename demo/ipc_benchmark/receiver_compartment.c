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
#include <unistd.h>
#include <inttypes.h>

#include "logging.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "types.h"

#include "portstatcounters.h"

extern cheri_riscv_hpms start_hpms;
extern cheri_riscv_hpms end_hpms;
/*-----------------------------------------------------------*/

void queueReceiveTask( void * pvParameters );
void __attribute__ ((noinline)) externFunc( void * pvParameters );

void externFunc( void * pvParameters ) {
    end_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    end_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
}

void externFault( void * pvParameters ) {
    start_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    start_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
    __asm__ volatile ("li a7, -1; ecall");
    //*((volatile int *) 0) = 0;
}

void queueReceiveTask( void * pvParameters )
{
#if (configENABLE_MPU == 1 && configMPU_COMPARTMENTALIZATION == 0)
    BaseType_t xRunningPrivileged = xPortRaisePrivilege();
#endif

    portDISABLE_INTERRUPTS();

#if (configENABLE_MPU == 1 && configMPU_COMPARTMENTALIZATION == 0)
    vPortResetPrivilege( xRunningPrivileged );
#endif

    UBaseType_t xTotalSize = IPC_TOTAL_SIZE;
    UBaseType_t xBufferSize = IPC_BUFFER_SIZE;
    QueueHandle_t* xQueue = NULL;
    IPCType_t xIPCMode = IPC_MODE;

    IPCTaskParams_t * params = ( IPCTaskParams_t * ) pvParameters;

    if( params != NULL )
    {
        xBufferSize = params->xBufferSize;
        xTotalSize = params->xTotalSize;
        xQueue = params->xQueue;
        xIPCMode = params->xIPCMode;
    }

    if (  xIPCMode == NOTIFICATIONS || xIPCMode == ALL ) {

        for( int i = 0; i < DISCARD_RUNS; i++ )
        {
            ulTaskNotifyTake( pdFALSE, portMAX_DELAY );
        }

        ulTaskNotifyTake( pdFALSE, portMAX_DELAY );

        PortStatCounters_ReadAll(&end_hpms);
        PortStatCounters_DiffAll(&start_hpms, &end_hpms, &end_hpms);

        log( "IPC Performance Results for: task notifications\n" );

        for (int i = 0; i < COUNTERS_NUM; i++) {
            log("HPM %s: %" PRIu64 "\n", hpm_names[i], end_hpms.counters[i]);
        }


        if ( xIPCMode == NOTIFICATIONS ) {
            /* Notify main task we are finished */
            xTaskNotifyGive( params->mainTask );

            vTaskDelete( NULL );
            while(1);
        }
    }

    // ---------------------------------------------------------------------- //
    // xIPCMode == QUEUES

    unsigned int cnt = xTotalSize / xBufferSize;

    if( !cnt )
    {
        log( "Invalid biffer sizes\n" );
    }

    //char * pReceiveBuffer = pvPortMalloc( xBufferSize );
    static char pReceiveBuffer[IPC_TOTAL_SIZE];

    if( !pReceiveBuffer )
    {
        log( "Failed to allocated a receive buffer of size %u\n", (unsigned) xBufferSize );
    }

    /* Zero the buffer to warm up the cache */
    memset( pReceiveBuffer, 0xa6, xBufferSize );

    #if VARY_QUEUE_SIZES
        for ( int y = 0; y <= log2 ( params->xTotalSize ); y++ )
        {

            for( int i = 0; i < DISCARD_RUNS; i++ )
            {
                xQueueReceive( xQueue[y], pReceiveBuffer, portMAX_DELAY );
            }

            /* Wait until something arrives in the queue - this task will block
             * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
             * FreeRTOSConfig.h. */
            xQueueReceive( xQueue[y], pReceiveBuffer, portMAX_DELAY );

            PortStatCounters_ReadAll(&end_hpms);
            PortStatCounters_DiffAll(&start_hpms, &end_hpms, &end_hpms);

            log( "IPC Performance Results for: queues: queue size: %lu\n", ( unsigned ) exp2( y ) );

            for (int i = 0; i < COUNTERS_NUM; i++) {
                log("HPM %s: %" PRIu64 "\n", hpm_names[i], end_hpms.counters[i]);
        }

        }

    #else
        for( int i = 0; i < DISCARD_RUNS; i++ )
        {
            xQueueReceive( xQueue[0], pReceiveBuffer, portMAX_DELAY );
        }

        for( int i = 0; i < cnt; i++ )
        {
            /* Wait until something arrives in the queue - this task will block
             * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
             * FreeRTOSConfig.h. */
            xQueueReceive( xQueue[0], pReceiveBuffer, portMAX_DELAY );
        }

        PortStatCounters_ReadAll(&end_hpms);
        PortStatCounters_DiffAll(&start_hpms, &end_hpms, &end_hpms);

        log( "IPC Performance Results for: queues: buffer size: %lu: total size: %lu\n",
             xBufferSize, xTotalSize
             );

        for (int i = 0; i < COUNTERS_NUM; i++) {
            log("HPM %s: %" PRIu64 "\n", hpm_names[i], end_hpms.counters[i]);
        }
    #endif

    #if VARY_BUFFER_SIZES
        for( int i = 0; i < DISCARD_RUNS; i++ )
        {
            xQueueReceive( xQueue[0], pReceiveBuffer, portMAX_DELAY );
        }

        for( int buffsize = 2; buffsize <= xTotalSize; buffsize *=2 ) {
            cnt = xTotalSize / buffsize;

            PortStatCounters_ReadAll(&start_hpms);

            for( int i = 0; i < cnt; i++ )
            {
                xQueueReceive( xQueue[(int) log2(buffsize)], pReceiveBuffer, portMAX_DELAY );
            }

            PortStatCounters_ReadAll(&end_hpms);
            PortStatCounters_DiffAll(&start_hpms, &end_hpms, &end_hpms);

            log( "IPC Performance Results for: queues: buffer size: %lu: total size: %lu\n",
                 buffsize, xTotalSize
                 );

            for (int i = 0; i < COUNTERS_NUM; i++) {
                log("HPM %s: %" PRIu64 "\n", hpm_names[i], end_hpms.counters[i]);
            }
        }
    #endif

    //vPortFree( pReceiveBuffer );
    /* Notify main task we are finished */
    xTaskNotifyGive( params->mainTask );
    vTaskDelete( NULL );

    while( 1 )
    {
    }
}
/*-----------------------------------------------------------*/

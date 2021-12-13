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

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "types.h"

#include "portstatcounters.h"

extern cheri_riscv_hpms start_hpms;
extern cheri_riscv_hpms end_hpms;

void queueSendTask( void * pvParameters );

static void __attribute__ ((noinline)) local( void * pvParameters );
void __attribute__ ((noinline)) localFunc( void * pvParameters );
void __attribute__ ((noinline)) externFunc( void * pvParameters );
void __attribute__ ((noinline)) externFault( void * pvParameters );

void ecall( void );
void callLocal( void * pvParameters );
void callFault( void * pvParameters );
void callSameCompartment( void * pvParameters );
void callExternalCompartment( void * pvParameters );

static void local( void * pvParameters ) {
    end_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    end_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
}

void localFunc( void * pvParameters ) {
    end_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    end_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
}

void callFault( void * pvParameters ) {
    for( int i = 0; i < DISCARD_RUNS; i++ ) {
        externFault(pvParameters);
        end_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
        end_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
    }

    externFault(pvParameters);
    end_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    end_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);

    log( "IPC Performance Results for: compartment faults\n");
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_CYCLE], end_hpms.counters[COUNTER_CYCLE] - start_hpms.counters[COUNTER_CYCLE]);
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_INSTRET], end_hpms.counters[COUNTER_INSTRET] - start_hpms.counters[COUNTER_INSTRET]);
}

void ecall( void ) {

    for( int i = 0; i < DISCARD_RUNS; i++ ) {
        start_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
        start_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
        asm volatile("li a7, 1; ecall");
        end_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
        end_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
    }

    start_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
    start_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    asm volatile("li a7, 1; ecall");
    end_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    end_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);

    log( "IPC Performance Results for: ecall\n");
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_CYCLE], end_hpms.counters[COUNTER_CYCLE] - start_hpms.counters[COUNTER_CYCLE]);
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_INSTRET], end_hpms.counters[COUNTER_INSTRET] - start_hpms.counters[COUNTER_INSTRET]);
}

void callLocal( void * pvParameters ) {
    for( int i = 0; i < DISCARD_RUNS; i++ ) {
        start_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
        start_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
        local(pvParameters);
    }

    start_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
    start_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    local(pvParameters);

    log( "IPC Performance Results for: local function call\n" );
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_CYCLE], end_hpms.counters[COUNTER_CYCLE] - start_hpms.counters[COUNTER_CYCLE]);
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_INSTRET], end_hpms.counters[COUNTER_INSTRET] - start_hpms.counters[COUNTER_INSTRET]);
}

void callSameCompartment( void * pvParameters ) {

    for( int i = 0; i < DISCARD_RUNS; i++ ) {
        start_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
        start_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
        localFunc(pvParameters);
    }

    start_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
    start_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    localFunc(pvParameters);

    log( "IPC Performance Results for: same compartment call\n" );
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_CYCLE], end_hpms.counters[COUNTER_CYCLE] - start_hpms.counters[COUNTER_CYCLE]);
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_INSTRET], end_hpms.counters[COUNTER_INSTRET] - start_hpms.counters[COUNTER_INSTRET]);
}

void callExternalCompartment( void * pvParameters ) {

    for( int i = 0; i < DISCARD_RUNS; i++ ) {
        start_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
        start_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
        externFunc(pvParameters);
    }

    start_hpms.counters[COUNTER_CYCLE] = portCounterGet(COUNTER_CYCLE);
    start_hpms.counters[COUNTER_INSTRET] = portCounterGet(COUNTER_INSTRET);
    externFunc(pvParameters);

    log( "IPC Performance Results for: compartment switch call\n" );
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_CYCLE], end_hpms.counters[COUNTER_CYCLE] - start_hpms.counters[COUNTER_CYCLE]);
    log("HPM %s: %" PRIu64 "\n", hpm_names[COUNTER_INSTRET], end_hpms.counters[COUNTER_INSTRET] - start_hpms.counters[COUNTER_INSTRET]);
}

void queueSendTask( void * pvParameters )
{
    BaseType_t xReturned;
    UBaseType_t xTotalSize = IPC_TOTAL_SIZE;
    UBaseType_t xBufferSize = IPC_BUFFER_SIZE;
    QueueHandle_t* xQueue = NULL;
    IPCType_t xIPCMode = IPC_MODE;

    IPCTaskParams_t * params = ( IPCTaskParams_t * ) pvParameters;

#if (configENABLE_MPU == 1 && configMPU_COMPARTMENTALIZATION == 0)
    BaseType_t xRunningPrivileged = xPortRaisePrivilege();
#endif

    portDISABLE_INTERRUPTS();

#if (configENABLE_MPU == 1 && configMPU_COMPARTMENTALIZATION == 0)
    vPortResetPrivilege( xRunningPrivileged );
#endif

    if( params != NULL )
    {
        xBufferSize = params->xBufferSize;
        xTotalSize = params->xTotalSize;
        xQueue = params->xQueue;
        xIPCMode = params->xIPCMode;
    }

    // Microbenchmarks
    ecall();
    #if (configCHERI_COMPARTMENTALIZATION && configCHERI_COMPARTMENTALIZATION_MODE == 1) || \
        (configMPU_COMPARTMENTALIZATION && configMPU_COMPARTMENTALIZATION_MODE == 1)
        callFault(pvParameters);
    #endif
    callLocal(pvParameters);
    callSameCompartment(pvParameters);
    callExternalCompartment(pvParameters);

    if (  xIPCMode == NOTIFICATIONS  || xIPCMode == ALL ) {

        for( int i = 0; i < DISCARD_RUNS; i++ )
        {
            xTaskNotifyGive( params->receiverTask );
        }

        PortStatCounters_ReadAll(&start_hpms);

        xTaskNotifyGive( params->receiverTask );

        if ( xIPCMode == NOTIFICATIONS ) {
            vTaskDelete( NULL );
            while(1);
        }
    }

    // ---------------------------------------------------------------------- //
    // xIPCMode == QUEUES

    unsigned int cnt = xTotalSize / xBufferSize;

    static char pBufferToSend[IPC_TOTAL_SIZE];

    if( !pBufferToSend )
    {
        log( "Failed to allocated a send buffer of size %u\n", (unsigned) xBufferSize );
    }

    /* Warm up the cache */
    memset( pBufferToSend, 0x6a, xBufferSize );

    #if VARY_QUEUE_SIZES
        /* Send to the queue - causing the queue receive task to unblock
         * 0 is used as the block time so the sending operation
         * will not block - it shouldn't need to block as the queue should always
         * be empty at this point in the code. */
        for ( int y = 0; y <= log2 ( params->xTotalSize ); y++ ) {
            for( int i = 0; i < DISCARD_RUNS; i++ )
            {
                xReturned = xQueueSend( xQueue[y], pBufferToSend, 0U );
                configASSERT( xReturned == pdPASS );
            }

            PortStatCounters_ReadAll(&start_hpms);

            xReturned = xQueueSend( xQueue[y], pBufferToSend, 0U );
            configASSERT( xReturned == pdPASS );
        }
    #else
        for( int i = 0; i < DISCARD_RUNS; i++ )
        {
            xReturned = xQueueSend( xQueue[0], pBufferToSend, 0U );
            configASSERT( xReturned == pdPASS );
        }

        PortStatCounters_ReadAll(&start_hpms);

        for( int i = 0; i < cnt; i++ )
        {
            /* Send to the queue - causing the queue receive task to unblock
             * 0 is used as the block time so the sending operation
             * will not block - it shouldn't need to block as the queue should always
             * be empty at this point in the code. */
            xReturned = xQueueSend( xQueue[0], pBufferToSend, 0U );
            configASSERT( xReturned == pdPASS );
        }
    #endif

    #if VARY_BUFFER_SIZES
        for( int i = 0; i < DISCARD_RUNS; i++ )
        {
            xReturned = xQueueSend( xQueue[0], pBufferToSend, 0U );
            configASSERT( xReturned == pdPASS );
        }


        for( int buffsize = 2; buffsize <= xTotalSize; buffsize *=2 ) {
            cnt = xTotalSize / buffsize;

            PortStatCounters_ReadAll(&start_hpms);

            for( int i = 0; i < cnt; i++ )
            {
                /* Send to the queue - causing the queue receive task to unblock
                 * 0 is used as the block time so the sending operation
                 * will not block - it shouldn't need to block as the queue should always
                 * be empty at this point in the code. */
                xReturned = xQueueSend( xQueue[(int) log2(buffsize)], pBufferToSend, 0U );
                configASSERT( xReturned == pdPASS );
            }
        }
    #endif

    //vPortFree( pBufferToSend );
    vTaskDelete( NULL );

    while( 1 )
    {
    }
}

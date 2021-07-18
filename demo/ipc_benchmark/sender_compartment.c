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

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "types.h"

#include "portstatcounters.h"

extern cheri_riscv_hpms start_hpms;

void queueSendTask( void * pvParameters );

void queueSendTask( void * pvParameters )
{
    BaseType_t xReturned;
    UBaseType_t xTotalSize = IPC_TOTAL_SIZE;
    UBaseType_t xBufferSize = IPC_BUFFER_SIZE;
    QueueHandle_t* xQueue = NULL;
    IPCType_t xIPCMode = IPC_MODE;

    IPCTaskParams_t * params = ( IPCTaskParams_t * ) pvParameters;

    portDISABLE_INTERRUPTS();

    if( params != NULL )
    {
        xBufferSize = params->xBufferSize;
        xTotalSize = params->xTotalSize;
        xQueue = params->xQueue;
        xIPCMode = params->xIPCMode;
    }

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

    char * pBufferToSend = pvPortMalloc( xBufferSize );

    if( !pBufferToSend )
    {
        log( "Failed to allocated a send buffer of size %d\n", xBufferSize );
    }

    /* Warm up the cache */
    memset( pBufferToSend, 0x6a, xBufferSize );

    #if VARY_QUEUE_SIZES
        for( int i = 0; i < DISCARD_RUNS; i++ )
        {
            for ( int y = 0; y <= log2 ( params->xTotalSize ); y++ ) {
                xReturned = xQueueSend( xQueue[y], pBufferToSend, 0U );
                configASSERT( xReturned == pdPASS );
            }
        }

        /* Send to the queue - causing the queue receive task to unblock
         * 0 is used as the block time so the sending operation
         * will not block - it shouldn't need to block as the queue should always
         * be empty at this point in the code. */
        for ( int y = 0; y <= log2 ( params->xTotalSize ); y++ ) {

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

    vPortFree( pBufferToSend );
    vTaskDelete( NULL );

    while( 1 )
    {
    }
}

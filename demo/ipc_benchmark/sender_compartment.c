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

typedef struct taskParams
{
    UBaseType_t xBufferSize;
    UBaseType_t xTotalSize;
    QueueHandle_t xQueue;
    TaskHandle_t mainTask;
} IPCTaskParams_t;

extern uint64_t xStartTime;

void queueSendTask( void * pvParameters );

void queueSendTask( void * pvParameters )
{
    BaseType_t xReturned;
    UBaseType_t xTotalSize = IPC_TOTAL_SIZE;
    UBaseType_t xBufferSize = IPC_BUFFER_SIZE;
    QueueHandle_t xQueue = NULL;

    IPCTaskParams_t * params = ( IPCTaskParams_t * ) pvParameters;

    if( params != NULL )
    {
        xBufferSize = params->xBufferSize;
        xTotalSize = params->xTotalSize;
        xQueue = params->xQueue;
    }

    unsigned int cnt = xTotalSize / xBufferSize;

    char * pBufferToSend = pvPortMalloc( xBufferSize );

    if( !pBufferToSend )
    {
        printf( "Failed to allocated a send buffer of size %d\n", xBufferSize );
    }

    /* Zero the buffer to warm up the cache */
    memset( pBufferToSend, 0, xBufferSize );

    for( int i = 0; i < DISCARD_RUNS; i++ )
    {
        xReturned = xQueueSend( xQueue, pBufferToSend, 0U );
        configASSERT( xReturned == pdPASS );
    }

    xStartTime = get_cycle_count();

    for( int i = 0; i < cnt; i++ )
    {
        /* Send to the queue - causing the queue receive task to unblock
         * 0 is used as the block time so the sending operation
         * will not block - it shouldn't need to block as the queue should always
         * be empty at this point in the code. */
        xReturned = xQueueSend( xQueue, pBufferToSend, 0U );
        configASSERT( xReturned == pdPASS );
    }

    vPortFree( pBufferToSend );
    vTaskDelete( NULL );

    while( 1 )
    {
    }
}

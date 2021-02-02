/*
   FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
   All rights reserved

   VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

   This file is part of the FreeRTOS distribution.

   FreeRTOS is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License (version 2) as published by the
   Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

 ***************************************************************************
 >>!   NOTE: The modification to the GPL is included to allow you to     !<<
 >>!   distribute a combined work that includes FreeRTOS without being   !<<
 >>!   obliged to provide the source code for proprietary components     !<<
 >>!   outside of the FreeRTOS kernel.                                   !<<
 ***************************************************************************

 FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  Full license text is available on the following
link: http://www.freertos.org/a00114.html

 ***************************************************************************
 *                                                                       *
 *    FreeRTOS provides completely free yet professionally developed,    *
 *    robust, strictly quality controlled, supported, and cross          *
 *    platform software that is more than just the market leader, it     *
 *    is the industry's de facto standard.                               *
 *                                                                       *
 *    Help yourself get started quickly while simultaneously helping     *
 *    to support the FreeRTOS project by purchasing a FreeRTOS           *
 *    tutorial book, reference manual, or both:                          *
 *    http://www.FreeRTOS.org/Documentation                              *
 *                                                                       *
 ***************************************************************************

http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
the FAQ page "My application does not run, what could be wrong?".  Have you
defined configASSERT()?

http://www.FreeRTOS.org/support - In return for receiving this top quality
embedded software for free we request you assist our global community by
participating in the support forum.

http://www.FreeRTOS.org/training - Investing in training allows your team to
be as productive as possible as early as possible.  Now you can receive
FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
Ltd, and the world's leading authority on the world's leading RTOS.

http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
including FreeRTOS+Trace - an indispensable productivity tool, a DOS
compatible FAT file system, and our tiny thread aware UDP/IP stack.

http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
licenses offer ticketed support, indemnification and commercial middleware.

http://www.SafeRTOS.com - High Integrity Systems also provide a safety
engineered and independently SIL3 certified version for use in safety and
mission critical applications that require provable dependability.

1 tab == 4 spaces!
*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

/* FreeRTOS includes. */
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* FreeRTOS+CLI includes */
#include "FreeRTOS_CLI.h"

/* Demo app includes. */
#include "ModbusServer.h"
#include "ModbusDemoConstants.h"

/* Modbus includes. */
#include <modbus/modbus.h>
#include <modbus/modbus-helpers.h>

/* Microbenchmark includes */
#if defined( MODBUS_MICROBENCHMARK )
#include "microbenchmark.h"
#endif

/* Modbus object capability includes */
#if defined(MODBUS_OBJECT_CAPS) || defined(MODBUS_OBJECT_CAPS_STUBS)
#include "modbus_object_caps.h"
#endif

/* Modbus network capability includes */
#if defined(MODBUS_NETWORK_CAPS)
#include "modbus_network_caps.h"
#endif

/*-----------------------------------------------------------*/

/*
 * The task that runs the Modbus server.
 */
static void prvModbusServerTask( void *pvParameters );

/*
 * Initialise the Modbus server (e.g., state and context).
 */
static void prvModbusServerInitialization( uint16_t usPort );

/*
 * Open and configure the TCP socket.
 */
static Socket_t prvOpenTCPServerSocket( uint16_t usPort );

/*
 * Processes a Modbus request.
 */
static BaseType_t prvProcessModbusRequest(const uint8_t *req, const int req_length,
        uint8_t *rsp, int *rsp_length);

/*
 * A connected socket is being closed.  Ensure the socket is closed at both ends
 * properly.
 */
static void prvGracefulShutdown( void );

/*-----------------------------------------------------------*/

/* Structure to hold queue messages (requests and responses). */
typedef struct _queue_msg_t
{
    int msg_length;
    uint8_t *msg;
} queue_msg_t;

/* The structure holding Modbus state information. */
static modbus_mapping_t *mb_mapping = NULL;

/* The structure holding Modbus context. */
static modbus_t *ctx = NULL;

/*-----------------------------------------------------------*/

void vStartModbusServerTask( uint16_t usStackSize, uint32_t ulPort, UBaseType_t uxPriority )
{
    xTaskCreate( prvModbusServerTask, "ModbusServer", usStackSize, ( void * ) ulPort, uxPriority, NULL );
}
/*-----------------------------------------------------------*/

void prvModbusServerTask( void *pvParameters )
{
    BaseType_t xReturned;
    char *pcModbusFunctionName;
    Socket_t xListeningSocket, xConnectedSocket;
    uint64_t ulCycleCountStart, ulCycleCountEnd, ulCycleCountDiff;

    /* The strange casting is to remove compiler warnings on 32-bit machines. */
    uint16_t usPort = ( uint16_t ) ( ( uint32_t ) pvParameters ) & 0xffffUL;

    /* buffers for comms with libmodbus */
    uint8_t *req = ( uint8_t * )pvPortMalloc( MODBUS_MAX_STRING_LENGTH * sizeof( uint8_t ) );
    int req_length = 0;
    uint8_t *rsp = ( uint8_t * )pvPortMalloc( MODBUS_MAX_STRING_LENGTH * sizeof( uint8_t ) );
    int rsp_length = 0;

#if defined( modbusEXEC_PERIOD_MS )
    uint32_t ulOverrunCount = 0;
    TickType_t xPreviousWakeTime = xTaskGetTickCount();
    TickType_t xTimeIncrement = pdMS_TO_TICKS( modbusEXEC_PERIOD_MS );
#endif

    /* Initialise the Modbus server state and context */
    prvModbusServerInitialization( usPort );

    /* Attempt to open the socket.  The port number is passed in the task
       parameter. */
    xListeningSocket = prvOpenTCPServerSocket( usPort );

    for( ;; )
    {
        /* Nothing for this task to do if the socket cannot be created. */
        if( xListeningSocket == FREERTOS_INVALID_SOCKET )
        {
            vTaskDelete( NULL );
        }

        /* Wait for an incoming connection. */
        xConnectedSocket = modbus_tcp_accept( ctx, &xListeningSocket );
        configASSERT( xConnectedSocket != NULL &&
                xConnectedSocket != FREERTOS_INVALID_SOCKET );

        /* Receive a request from the Modbus client. */
        req_length = modbus_receive( ctx, req );

#if defined( modbusNETWORK_DELAY_MS )
        /* For the macrobenchmark, we simulate network delay by blocking
         * after receiving a request and before sending a reply. */
        vTaskDelay( pdMS_TO_TICKS( modbusNETWORK_DELAY_MS ) );
#endif

        /* Process the socket as long as it remains connected. */
        while( req_length >= 0 )
        {
            /* get the modbus function name from the request */
            pcModbusFunctionName = modbus_get_function_name( ctx, req );

            /* Ensure access to mb_mapping and ctx cannot be interrupted while processing
             * a request from the client */
            taskENTER_CRITICAL();

            /* One of the microbenchmarks is to measure the time to process each
             * Modbus function, which we do by measuring cycle counts across
             * the call to prvProcessModbusRequest(), which includes a call
             * to modbus_process_request(). */

            /* get the cycle count before processing the request. */
            ulCycleCountStart = get_cycle_count();

            /* Process the request. */
            xReturned  = prvProcessModbusRequest( req, req_length, rsp, &rsp_length );
            configASSERT( xReturned != -1 );

            /* get the cycle count after processing the request. */
            ulCycleCountEnd = get_cycle_count();

            /* critical access to mb_mapping and ctx is finished, so it's safe to exit */
            taskEXIT_CRITICAL();

            /* calculate the cycle count difference */
            ulCycleCountDiff = ulCycleCountEnd - ulCycleCountStart;

#if defined( MODBUS_MICROBENCHMARK )
            /* Record the cycle count difference. */
            xMicrobenchmarkSample( REQUEST_PROCESSING, pcModbusFunctionName,
                    ulCycleCountDiff, pdTRUE );
#endif

#if defined( modbusNETWORK_DELAY_MS )
            /* For the macrobenchmark, we simulate network delay by blocking
             * after receiving a request and before sending a reply. */
            vTaskDelay( pdMS_TO_TICKS( modbusNETWORK_DELAY_MS ) );
#endif

            /* Reply to the Modbus client. */
            xReturned = modbus_reply( ctx, rsp, rsp_length );
            configASSERT( xReturned != -1 );

#if defined( modbusEXEC_PERIOD_MS )
            /* Check if we've overrun the execution period.  This might happen
             * when we first connect, or when running network capabilities
             * with a small execution period.
             *
             * If we overrun, we'll reset xPreviousWakeTime to set up the
             * next loop and then skip vTaskDelayUntil().  We will also set
             * ulCycleCountDiff to 0, which will be recorded as a
             * SPARE_PROCESSING SAMPLE, which can be easily filtered out
             * during data analysis. */
            if( xTaskGetTickCount() > xPreviousWakeTime + xTimeIncrement )
            {
                FreeRTOS_debug_printf( ( "Overrun execution period. Resetting. Overrun count: %d...\r\n", ulOverrunCount + 1 ) );
                FreeRTOS_debug_printf( ( "Modbus function: %s\r\n", pcModbusFunctionName ) );
                FreeRTOS_debug_printf( ( "xTaskGetTickCount() = %d\r\n", xTaskGetTickCount() ) );
                FreeRTOS_debug_printf( ( "xPreviousWakeTime = %d\r\n", xPreviousWakeTime ) );
                FreeRTOS_debug_printf( ( "xTimeIncrement = %d\r\n", xTimeIncrement ) );
                xPreviousWakeTime = xTaskGetTickCount();
                ulOverrunCount += 1;

                ulCycleCountDiff = 0;
            }
            else
            {
                /* One of the microbenchmarks is to measure idle or spare
                 * processing time, which we'll do by measuring cycle counts
                 * across a call to vTaskDelayUntil(). */

                /* get the cycle count before blocking. */
                ulCycleCountStart = get_cycle_count();

                /* Block until the next, fixed execution period */
                vTaskDelayUntil( &xPreviousWakeTime, xTimeIncrement );

                /* get the cycle count after blocking. */
                ulCycleCountEnd = get_cycle_count();

                /* calculate the cycle count difference */
                ulCycleCountDiff = ulCycleCountEnd - ulCycleCountStart;
            }

#if defined( MODBUS_MICROBENCHMARK )
            /* Save the difference in cycle count as a benchmarking sample. */
            xMicrobenchmarkSample( SPARE_PROCESSING, pcModbusFunctionName,
                    ulCycleCountDiff, pdTRUE );
#endif /* defined( MODBUS_MICROBENCHMARK ) */

#endif /* defined( modbusEXEC_PERIOD_MS ) */

            /* Receive the next request from the Modbus client. */
            req_length = modbus_receive( ctx, req );

#if defined( modbusNETWORK_DELAY_MS )
            /* For the macrobenchmark, we simulate network delay by blocking
             * after receiving a request and before sending a reply. */
            vTaskDelay( pdMS_TO_TICKS( modbusNETWORK_DELAY_MS ) );
#endif
        }

        /* Close the socket correctly. */
        prvGracefulShutdown();

#if defined( MODBUS_MICROBENCHMARK )
        /* Print microbenchmark samples to stdout and do not reopen the port */
        vPrintMicrobenchmarkSamples();
#endif
    }
}

/*-----------------------------------------------------------*/

static void prvGracefulShutdown( void )
{
    modbus_close(ctx);
}
/*-----------------------------------------------------------*/

static Socket_t prvOpenTCPServerSocket( uint16_t usPort )
{
    struct freertos_sockaddr xBindAddress;
    Socket_t xSocket;
    static const TickType_t xReceiveTimeOut = portMAX_DELAY;
    const BaseType_t xBacklog = 1;

    /* Attempt to open the socket. */
    xSocket = modbus_tcp_listen( ctx, xBacklog);
    configASSERT( xSocket != FREERTOS_INVALID_SOCKET );

    return xSocket;
}

/*-----------------------------------------------------------*/

static void prvModbusServerInitialization( uint16_t port )
{
    /* Allocate and populate the ctx structure.  Pass NULL for the ip,
     * since it isn't necessary for the server to know its own ip address. */
    ctx = modbus_new_tcp( NULL, port );
    if ( ctx == NULL )
    {
        fprintf( stderr, "Failed to allocate ctx: %s\r\n",
                modbus_strerror( errno ) );
        modbus_free( ctx );
        _exit( 0 );
    }

#ifdef NDEBUG
    modbus_set_debug( ctx, pdFALSE );
#else
    modbus_set_debug( ctx, pdTRUE );
#endif

    /* initialise state (mb_mapping) */
#if defined(MODBUS_OBJECT_CAPS) && !defined(MODBUS_OBJECT_CAPS_STUB)
    mb_mapping = modbus_mapping_new_start_address_object_caps(
            ctx,
            UT_BITS_ADDRESS, UT_BITS_NB,
            UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB,
            UT_REGISTERS_ADDRESS, UT_REGISTERS_NB_MAX,
            UT_INPUT_REGISTERS_ADDRESS, UT_INPUT_REGISTERS_NB );
#else
    mb_mapping = modbus_mapping_new_start_address(
            UT_BITS_ADDRESS, UT_BITS_NB,
            UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB,
            UT_REGISTERS_ADDRESS, UT_REGISTERS_NB_MAX,
            UT_INPUT_REGISTERS_ADDRESS, UT_INPUT_REGISTERS_NB );
#endif

    /* check for successful initialisation of state */
    if ( mb_mapping == NULL )
    {
        fprintf( stderr, "Failed to allocate the mapping: %s\r\n",
                modbus_strerror( errno ) );
        modbus_free( ctx );
        _exit( 0 );
    }

    /* display the state if DEBUG */
    if ( modbus_get_debug( ctx ) )
    {
        print_mb_mapping( mb_mapping );
    }

    /* Initialize coils */
    modbus_set_bits_from_bytes( mb_mapping->tab_input_bits, 0, UT_INPUT_BITS_NB,
            UT_INPUT_BITS_TAB );

    /* Initialize discrete inputs */
    for ( int i = 0; i < UT_INPUT_REGISTERS_NB; i++ )
    {
        mb_mapping->tab_input_registers[i] = UT_INPUT_REGISTERS_TAB[i];
    }

#if defined(MODBUS_NETWORK_CAPS)
    /* Initialise Macaroon */
    BaseType_t xReturned = 0;
    char *key = "a bad secret";
    char *id = "id for a bad secret";
    char *location = "https://www.modbus.com/macaroons/";
    xReturned = initialise_server_network_caps( ctx, location, key, id );
    if (xReturned == -1) {
        fprintf( stderr, "Failed to initialise server macaroon\r\n" );
        modbus_free( ctx );
        _exit( 0 );
    }
#endif
}

/*-----------------------------------------------------------*/

/**
 * Process a Modbus request from a client to a server.
 *
 * Returns the cycle count to process the request.
 */
static BaseType_t prvProcessModbusRequest(const uint8_t *req, const int req_length,
        uint8_t *rsp, int *rsp_length)
{
    BaseType_t xReturned;

    /**
     * Perform preprocessing for object or network capabilities
     * then perform the normal processing
     * NB order matters here:
     * - First reduce permissions on state
     * - Then verify the network capability, if appropriate
     * - Then perform the normal processing
     * NB A configuration without object or network capabilities can still
     * be compiled for a CHERI system, it just wont restrict the state before
     * processing the request.
     * */
#if defined(MODBUS_OBJECT_CAPS_STUB)
    /* this is only used to evaluate the overhead of calling a function */
    xReturned = modbus_preprocess_request_object_caps_stub(ctx, req, mb_mapping);
    configASSERT(xReturned != -1);
#elif defined(MODBUS_OBJECT_CAPS)
    xReturned = modbus_preprocess_request_object_caps(ctx, req, mb_mapping);
    configASSERT(xReturned != -1);
#endif

#if defined(MODBUS_NETWORK_CAPS)
    xReturned = modbus_preprocess_request_network_caps(ctx, req, mb_mapping);
    configASSERT(xReturned != -1);
#endif

    xReturned = modbus_process_request(ctx, req, req_length,
            rsp, rsp_length, mb_mapping);

    return xReturned;
}

/*-----------------------------------------------------------*/

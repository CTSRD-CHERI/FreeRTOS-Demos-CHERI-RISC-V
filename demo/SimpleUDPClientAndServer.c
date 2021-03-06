/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

/*
 * Creates two transmitting tasks and two receiving tasks.  The transmitting
 * tasks send values that are received by the receiving tasks.  One set of tasks
 * uses the standard API.  The other set of tasks uses the zero copy API.
 *
 * See the following web page for essential demo usage and configuration
 * details:
 * http://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/examples_FreeRTOS_simulator.html
 */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#include "SimpleUDPClientAndServer.h"

/*
 * Uses a socket to send data using the zero copy option.
 * prvSimpleZeroCopyServerTask() will receive the data.
 */
static void prvSimpleZeroCopyUDPClientTask( void * pvParameters );

/*
 * Uses a socket to receive the data sent by the prvSimpleZeroCopyUDPClientTask()
 * task.  Uses the zero copy option.
 */
static void prvSimpleZeroCopyServerTask( void * pvParameters );

/*-----------------------------------------------------------*/

#if __riscv_xlen == 64
    void vStartSimpleUDPClientServerTasks( uint16_t usStackSize,
                                           uint64_t ulPort,
                                           UBaseType_t uxPriority )
#else
    void vStartSimpleUDPClientServerTasks( uint16_t usStackSize,
                                           uint32_t ulPort,
                                           UBaseType_t uxPriority )
#endif
{
    /* Create the client and server tasks that do use the zero copy interface. */
    xTaskCreate( prvSimpleZeroCopyUDPClientTask, "SimpZCpyClnt", usStackSize, ( void * ) ( uintptr_t ) ( ulPort + 1 ), uxPriority, NULL );
    xTaskCreate( prvSimpleZeroCopyServerTask, "SimpZCpySrv", usStackSize, ( void * ) ( uintptr_t ) ( ulPort + 1 ), uxPriority + 1, NULL );
}
/*-----------------------------------------------------------*/

static void prvSimpleZeroCopyUDPClientTask( void * pvParameters )
{
    Socket_t xClientSocket;
    uint8_t * pucUDPPayloadBuffer;
    struct freertos_sockaddr xDestinationAddress;
    BaseType_t lReturned;
    uint32_t ulCount = 0UL, ulIPAddress;
    const uint32_t ulLoopsPerSocket = 10UL;
    const char * pcStringToSend = "Server received (using zero copy): Message number ";
    const TickType_t x150ms = 150UL / portTICK_PERIOD_MS;
    /* 15 is added to ensure the number, \r\n and terminating zero fit. */
    const size_t xStringLength = strlen( pcStringToSend ) + 15;

    /* Remove compiler warning about unused parameters. */
    ( void ) pvParameters;

    /* It is assumed that this task is not created until the network is up,
     * so the IP address can be obtained immediately.  store the IP address being
     * used in ulIPAddress.  This is done so the socket can send to a different
     * port on the same IP address. */
    FreeRTOS_GetAddressConfiguration( &ulIPAddress, NULL, NULL, NULL );

    /* This test sends to itself, so data sent from here is received by a server
     * socket on the same IP address.  Setup the freertos_sockaddr structure with
     * this nodes IP address, and the port number being sent to.  The strange
     * casting is to try and remove compiler warnings on 32 bit machines. */
    /*xDestinationAddress.sin_addr = ulIPAddress; // NOTE: we cannot really send packets to ourselves */
    xDestinationAddress.sin_addr = FreeRTOS_inet_addr_quick( configECHO_SERVER_ADDR0,
                                                             configECHO_SERVER_ADDR1,
                                                             configECHO_SERVER_ADDR2,
                                                             configECHO_SERVER_ADDR3 );
    #if defined( __clang__ )
    #else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    #endif
    xDestinationAddress.sin_port = ( uint16_t ) ( ( uint32_t ) pvParameters ) & 0xffffUL;
    #if defined( __clang__ )
    #else
    #pragma GCC diagnostic pop
    #endif

    xDestinationAddress.sin_port = FreeRTOS_htons( xDestinationAddress.sin_port );

    for( ; ; )
    {
        FreeRTOS_netstat();
        /* Create the socket. */
        xClientSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
        configASSERT( xClientSocket != FREERTOS_INVALID_SOCKET );

        /* The count is used to differentiate between different messages sent to
         * the server, and to break out of the do while loop below. */
        ulCount = 0UL;

        /* NOTE: No need to `connect` a UDP socket */
        /*FreeRTOS_connect( xClientSocket, &xDestinationAddress, sizeof( xDestinationAddress ) ); */

        do
        {
            FreeRTOS_debug_printf( ( "prvSimpleZeroCopyUDPClientTask: waiting for a buffer, ulCount = %lu\r\n", ( unsigned long ) ulCount ) );

            /* This task is going to send using the zero copy interface.  The
             * data being sent is therefore written directly into a buffer that is
             * passed into, rather than copied into, the FreeRTOS_sendto()
             * function.
             *
             * First obtain a buffer of adequate length from the IP stack into which
             * the string will be written.  Although a max delay is used, the actual
             * delay will be capped to ipconfigMAX_SEND_BLOCK_TIME_TICKS, hence
             * the do while loop is used to ensure a buffer is obtained. */
            do
            {
            } while( ( pucUDPPayloadBuffer = ( uint8_t * ) FreeRTOS_GetUDPPayloadBuffer( xStringLength, portMAX_DELAY ) ) == NULL );

            FreeRTOS_debug_printf( ( "prvSimpleZeroCopyUDPClientTask: got a buffer at %p\r\n", pucUDPPayloadBuffer ) );

            /* A buffer was successfully obtained.  Create the string that is
             * sent to the server.  First the string is filled with zeros as this will
             * effectively be the null terminator when the string is received at the other
             * end.  Note that the string is being written directly into the buffer
             * obtained from the IP stack above. */
            memset( ( void * ) pucUDPPayloadBuffer, 0x00, xStringLength );
            sprintf( ( char * ) pucUDPPayloadBuffer, "%s%lu\r\n", pcStringToSend, ( unsigned long ) ulCount );

            /* Pass the buffer into the send function.  ulFlags has the
             * FREERTOS_ZERO_COPY bit set so the IP stack will take control of the
             * buffer rather than copy data out of the buffer. */
            lReturned = FreeRTOS_sendto( xClientSocket,                                      /* The socket being sent to. */
                                         ( void * ) pucUDPPayloadBuffer,                     /* A pointer to the the data being sent. */
                                         strlen( ( const char * ) pucUDPPayloadBuffer ) + 1, /* The length of the data being sent - including the string's null terminator. */
                                         FREERTOS_ZERO_COPY,                                 /* ulFlags with the FREERTOS_ZERO_COPY bit set. */
                                         &xDestinationAddress,                               /* Where the data is being sent. */
                                         sizeof( xDestinationAddress ) );
            FreeRTOS_debug_printf( ( "prvSimpleZeroCopyUDPClientTask: FreeRTOS_sendto returned %lu\r\n", lReturned ) );

            if( lReturned == 0 )
            {
                /* The send operation failed, so this task is still responsible
                 * for the buffer obtained from the IP stack.  To ensure the buffer
                 * is not lost it must either be used again, or, as in this case,
                 * returned to the IP stack using FreeRTOS_ReleaseUDPPayloadBuffer().
                 * pucUDPPayloadBuffer can be safely re-used after this call. */
                FreeRTOS_debug_printf( ( "prvSimpleZeroCopyUDPClientTask: lReturned == 0, returing pucUDPPayloadBuffer\r\n" ) );
                FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucUDPPayloadBuffer );
            }
            else
            {
                /* The send was successful so the IP stack is now managing the
                 * buffer pointed to by pucUDPPayloadBuffer, and the IP stack will
                 * return the buffer once it has been sent.  pucUDPPayloadBuffer can
                 * be safely re-used. */
                FreeRTOS_debug_printf( ( "prvSimpleZeroCopyUDPClientTask: all well in the universe\r\n" ) );
            }

            ulCount++;
            vTaskDelay( pdMS_TO_TICKS( 1000 ) );
        } while( ( lReturned != FREERTOS_SOCKET_ERROR ) && ( ulCount < ulLoopsPerSocket ) );

        FreeRTOS_debug_printf( ( "prvSimpleZeroCopyUDPClientTask: closing and returning\r\n" ) );
        FreeRTOS_closesocket( xClientSocket );

        /* A short delay to prevent the messages scrolling off the screen too
         * quickly. */
        vTaskDelay( x150ms );
        vTaskDelay( pdMS_TO_TICKS( 500 ) );
    }
}
/*-----------------------------------------------------------*/

static void prvSimpleZeroCopyServerTask( void * pvParameters )
{
    int32_t lBytes;
    uint8_t * pucUDPPayloadBuffer;
    struct freertos_sockaddr xClient, xBindAddress;
    uint32_t xClientLength = sizeof( xClient ), ulIPAddress;
    Socket_t xListeningSocket;

    /* Just to prevent compiler warnings. */
    ( void ) pvParameters;

    /* Attempt to open the socket. */
    xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
    configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

    /* This test receives data sent from a different port on the same IP address.
     * Obtain the nodes IP address.  Configure the freertos_sockaddr structure with
     * the address being bound to.  The strange casting is to try and remove
     * compiler warnings on 32 bit machines.  Note that this task is only created
     * after the network is up, so the IP address is valid here. */
    FreeRTOS_GetAddressConfiguration( &ulIPAddress, NULL, NULL, NULL );
    xBindAddress.sin_addr = ulIPAddress;
    #if defined( __clang__ )
    #else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    #endif
    xBindAddress.sin_port = ( uint16_t ) ( ( uint32_t ) pvParameters ) & 0xffffUL;
    xBindAddress.sin_port = FreeRTOS_htons( xBindAddress.sin_port );

    /* Bind the socket to the port that the client task will send to. */
    FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );

    for( ; ; )
    {
        /* Receive data on the socket.  ulFlags has the zero copy bit set
         * (FREERTOS_ZERO_COPY) indicating to the stack that a reference to the
         * received data should be passed out to this task using the second
         * parameter to the FreeRTOS_recvfrom() call.  When this is done the
         * IP stack is no longer responsible for releasing the buffer, and
         * the task *must* return the buffer to the stack when it is no longer
         * needed.  By default the block time is portMAX_DELAY. */
        lBytes = FreeRTOS_recvfrom( xListeningSocket, ( void * ) &pucUDPPayloadBuffer, 0, FREERTOS_ZERO_COPY, &xClient, &xClientLength );

        /* Print the received characters. */
        if( lBytes > 0 )
        {
            FreeRTOS_printf( ( "prvSimpleZeroCopyServerTask: received %ld bytes\r\n", ( long ) lBytes ) );

            /* It is expected to receive one more byte than the string length as
             * the NULL terminator is also transmitted. */
            /*configASSERT( lBytes == ( ( BaseType_t ) strlen( ( const char * ) pucUDPPayloadBuffer ) + 1 ) ); */
            lBytes = FreeRTOS_sendto( xListeningSocket,                             /* The socket being sent to. */
                                      ( void * ) pucUDPPayloadBuffer,               /* A pointer to the the data being sent. */
                                      lBytes,
                                      FREERTOS_ZERO_COPY,                           /* ulFlags with the FREERTOS_ZERO_COPY bit set. */
                                      &xClient,                                     /* Where the data is being sent. */
                                      sizeof( xClientLength ) );
            FreeRTOS_printf( ( "prvSimpleZeroCopyServerTask: FreeRTOS_sendto returned %ld\r\n", ( long ) lBytes ) );
        }

        if( lBytes >= 0 )
        {
            /* The buffer *must* be freed once it is no longer needed. */
            FreeRTOS_ReleaseUDPPayloadBuffer( pucUDPPayloadBuffer );
        }
    }
}

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
 * NOTE 1:  This project provides a demo of the Modbus industrial protocol.
 *
 * NOTE 2:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, and functions
 * required to configure the hardware are defined in main.c.
 ******************************************************************************
 *
 * main_modbus() runs a demo application using the modbus protocol.
 *
 * It creates a server and client tasks that communicate using queue.  The server
 * task spins up a separate task for critical processing and communicates
 * with the critical processing task via a queue as well.
 *
 * The client generates a request, which is sent to the server.  The server
 * sends the request to the critical processing task, which executes the request
 * and constructs a reply, which it returns to the server.  The server send
 * the reply to the client.
 *
 * The demo is constructed to be compiled with and without CHERI capabilities.
 *
 * The demo is constructed to incorporate object capabilities (CHERI_LAYER)
 * and network capabilities (MACAROONS_LAYER).
 *
 * The demo is constructed to support microbenchmarking of the
 * critical processing task.
 * */

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* Modbus includes. */
#include <modbus/modbus.h>
#include <modbus/modbus-helpers.h>

/* Demo includes */
#include "modbus_demo.h"
#include "modbus_server.h"
#include "modbus_client.h"

#if defined(CHERI_LAYER)
/* Modbus CHERI includes */
#include "modbus_cheri.h"
#endif

#if defined(MACAROONS_LAYER)
/* Macaroons includes */
#include "modbus_macaroons.h"
#endif

/*-----------------------------------------------------------*/

/* The default IP and MAC address used by the demo.  The address configuration
defined here will be used if ipconfigUSE_DHCP is 0, or if ipconfigUSE_DHCP is
1 but a DHCP server could not be contacted.  See the online documentation for
more information. */
static const uint8_t ucIPAddress[ 4 ] = { configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 };
static const uint8_t ucNetMask[ 4 ] = { configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 };
static const uint8_t ucGatewayAddress[ 4 ] = { configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3 };
static const uint8_t ucDNSServerAddress[ 4 ] = { configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3 };

/* Default MAC address configuration.  The demo creates a virtual network
connection that uses this MAC address by accessing the raw Ethernet data
to and from a real network connection on the host PC.  See the
configNETWORK_INTERFACE_TO_USE definition for information on how to configure
the real network connection to use. */
static const uint8_t ucMACAddress[ 6 ] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

/* The default modbus server port is 502; however, the client and server
 * are running as two tasks on the same instance of FreeRTOS running
 * on QEMU.  The client and server communicate via the loopback interface
 * on the host.  Currently, the QEMU port 502 is mapped to port 1502 on the
 * host, so the client will send to localhost:1502 and the server will
 * be listening on localhost:502 */
const int modbus_port = 502;
const int modbus_port_mapped = 1502;
const char *localhost_ip = "127.0.0.1";

/*-----------------------------------------------------------*/

void main_modbus(void)
{
    /* Initialise the network interface.
     * ***NOTE*** Tasks that use the network are created in the network event hook
     * when th e network is connected and ready for use (see the definition of
     * vApplicationIPNetworkEventHook() below).  The address values passed in here
     * are used if ipconfigUSE_DHCP is set to 0, or if ipconfigUSE_DHCP is set to 1
     * but a DHCP server cannot be	contacted. */
    FreeRTOS_debug_printf( ( "FreeRTOS_IPInit\n" ) );
    FreeRTOS_IPInit( ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress );
}

/*-----------------------------------------------------------*/

/* Called by FreeRTOS+TCP when the network connects or disconnects.  Disconnect
events are only received if implemented in the MAC driver. */
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
    uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
    char cBuffer[ 16 ];
    static BaseType_t xTasksAlreadyCreated = pdFALSE;

	/* If the network has just come up...*/
	if( eNetworkEvent == eNetworkUp )
	{
		/* Create the tasks that use the IP stack if they have not already been
		created. */
		if( xTasksAlreadyCreated == pdFALSE )
		{
            /* Initialise the server and client */
            FreeRTOS_debug_printf( ( "vServerInitialization\n" ) );
            vServerInitialization(localhost_ip, modbus_port);
            FreeRTOS_debug_printf( ( "vClientInitialization\n" ) );
            vClientInitialization(localhost_ip, modbus_port_mapped);

            /*
            * Start the client and server tasks as described in the comments at the top of this
            * file.
            */
            xTaskCreate(vClientTask,                 /* The function that implements the task. */
                      "Client",                      /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                      configMINIMAL_STACK_SIZE * 2U, /* The size of the stack to allocate to the task. */
                      NULL,                          /* The parameter passed to the task - not used in this case. */
                      modbusCLIENT_TASK_PRIORITY,      /* The priority assigned to the task. */
                      NULL);                         /* The task handle is not required, so NULL is passed. */

            xTaskCreate(vServerTask,                 /* The function that implements the task. */
                      "Server",                      /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                      configMINIMAL_STACK_SIZE * 2U, /* The size of the stack to allocate to the task. */
                      NULL,                          /* The parameter passed to the task - not used in this case. */
                      modbusSERVER_TASK_PRIORITY,      /* The priority assigned to the task. */
                      NULL);                         /* The task handle is not required, so NULL is passed. */

            xTasksAlreadyCreated = pdTRUE;
        }

		/* Print out the network configuration, which may have come from a DHCP
		server. */
		FreeRTOS_GetAddressConfiguration( &ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress );
		FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
		FreeRTOS_printf( ( "\r\n\r\nIP Address: %s\r\n", cBuffer ) );

		FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
		FreeRTOS_printf( ( "Subnet Mask: %s\r\n", cBuffer ) );

		FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
		FreeRTOS_printf( ( "Gateway Address: %s\r\n", cBuffer ) );

		FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
		FreeRTOS_printf( ( "DNS Server Address: %s\r\n\r\n\r\n", cBuffer ) );
	}
}

/*-----------------------------------------------------------*/

/* Called automatically when a reply to an outgoing ping is received. */
void vApplicationPingReplyHook( ePingReplyStatus_t eStatus, uint16_t usIdentifier )
{
static const char *pcSuccess = "Ping reply received - ";
static const char *pcInvalidChecksum = "Ping reply received with invalid checksum - ";
static const char *pcInvalidData = "Ping reply received with invalid data - ";

	switch( eStatus )
	{
		case eSuccess	:
			FreeRTOS_printf( ( pcSuccess ) );
			break;

		case eInvalidChecksum :
			FreeRTOS_printf( ( pcInvalidChecksum ) );
			break;

		case eInvalidData :
			FreeRTOS_printf( ( pcInvalidData ) );
			break;

		default :
			/* It is not possible to get here as all enums have their own
			case. */
			break;
	}

	FreeRTOS_printf( ( "identifier %d\r\n", ( int ) usIdentifier ) );

	/* Prevent compiler warnings in case FreeRTOS_debug_printf() is not defined. */
	( void ) usIdentifier;
}

/*-----------------------------------------------------------*/

/*
 * Callback that provides the inputs necessary to generate a randomized TCP
 * Initial Sequence Number per RFC 6528.  THIS IS ONLY A DUMMY IMPLEMENTATION
 * THAT RETURNS A PSEUDO RANDOM NUMBER SO IS NOT INTENDED FOR USE IN PRODUCTION
 * SYSTEMS.
 */
extern uint32_t ulApplicationGetNextSequenceNumber(uint32_t ulSourceAddress,
	uint16_t usSourcePort,
	uint32_t ulDestinationAddress,
	uint16_t usDestinationPort)
{
	(void)ulSourceAddress;
	(void)usSourcePort;
	(void)ulDestinationAddress;
	(void)usDestinationPort;

	return uxRand();
}

/*-----------------------------------------------------------*/

void vPrintTx(uint8_t *msg, int msg_length)
{
  printf("TX:\t");
  for (int i = 0; i < msg_length; ++i)
  {
    printf("[%.2X]", msg[i]);
  }
  printf("\n\n");
}

/*-----------------------------------------------------------*/

void vPrintRx(uint8_t *msg, int msg_length)
{
  printf("RX:\t");
  for (int i = 0; i < msg_length; ++i)
  {
    printf("<%.2X>", msg[i]);
  }
  printf("\n\n");
}

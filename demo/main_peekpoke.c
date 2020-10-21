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

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

/* FreeRTOS  includes. */
#include "FreeRTOS.h"
#include "task.h"

/* IP stack includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* Application includes */
#include "SimpleUDPClientAndServer.h"
#include "SimpleTCPEchoServer.h"
#include "TCPEchoClient_SingleTasks.h"

/* Peek-poke stuff */
#include "peekpoke.h"

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri-utility.h>
#endif

/* Simple UDP client and server task parameters. */
#define mainSIMPLE_UDP_CLIENT_SERVER_TASK_PRIORITY (tskIDLE_PRIORITY)
#define mainSIMPLE_UDP_CLIENT_SERVER_PORT (5005UL)
#define mainSIMPLE_UDP_CLIENT_SERVER_STACK_SIZE (configMINIMAL_STACK_SIZE * 10)

/* Echo client task parameters - used for both TCP and UDP echo clients. */
#define mainECHO_CLIENT_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 10)
#define mainECHO_CLIENT_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

/* Echo server task parameters. */
#define mainECHO_SERVER_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 10)
#define mainECHO_SERVER_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

/* Define a name that will be used for LLMNR and NBNS searches. */
#define mainHOST_NAME "RTOSDemo"
#define mainDEVICE_NICK_NAME "fpga_demo"

/* Set the following constants to 1 or 0 to define which tasks to include and
   exclude:

   mainCREATE_SIMPLE_UDP_CLIENT_SERVER_TASKS:  When set to 1 two UDP client tasks
   and two UDP server tasks are created.  The clients talk to the servers.  One set
   of tasks use the standard sockets interface, and the other the zero copy sockets
   interface.  These tasks are self checking and will trigger a configASSERT() if
   they detect a difference in the data that is received from that which was sent.
   As these tasks use UDP, and can therefore loose packets, they will cause
   configASSERT() to be called when they are run in a less than perfect networking
   environment.

   mainCREATE_TCP_ECHO_TASKS_SINGLE:  When set to 1 a set of tasks are created that
   send TCP echo requests to the standard echo port (port 7), then wait for and
   verify the echo reply, from within the same task (Tx and Rx are performed in the
   same RTOS task).  The IP address of the echo server must be configured using the
   configECHO_SERVER_ADDR0 to configECHO_SERVER_ADDR3 constants in
   FreeRTOSConfig.h.

   mainCREATE_TCP_ECHO_SERVER_TASK:  When set to 1 a task is created that accepts
   connections on the standard echo port (port 7), then echos back any data
   received on that connection.
*/
#ifndef mainCREATE_SIMPLE_UDP_CLIENT_SERVER_TASKS
#define mainCREATE_SIMPLE_UDP_CLIENT_SERVER_TASKS 0
#endif

#ifndef mainCREATE_TCP_ECHO_TASKS_SINGLE
#define mainCREATE_TCP_ECHO_TASKS_SINGLE 0
#endif

#ifndef mainCREATE_TCP_ECHO_SERVER_TASK
#define mainCREATE_TCP_ECHO_SERVER_TASK 0
#endif

/*
 * Just seeds the simple pseudo random number generator.
 */
static void prvSRand(UBaseType_t ulSeed);

/*
 * Miscellaneous initialisation including preparing the logging and seeding the
 * random number generator.
 */
static void prvMiscInitialisation(void);

uint32_t ulApplicationGetNextSequenceNumber(uint32_t ulSourceAddress,
											uint16_t usSourcePort,
											uint32_t ulDestinationAddress,
											uint16_t usDestinationPort);

/* The default IP and MAC address used by the demo.  The address configuration
   defined here will be used if ipconfigUSE_DHCP is 0, or if ipconfigUSE_DHCP is
   1 but a DHCP server could not be contacted.  See the online documentation for
   more information. */
static const uint8_t ucIPAddress[4] = {configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3};
static const uint8_t ucNetMask[4] = {configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3};
static const uint8_t ucGatewayAddress[4] = {configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3};
static const uint8_t ucDNSServerAddress[4] = {configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3};

/* Set the following constant to pdTRUE to log using the method indicated by the
   name of the constant, or pdFALSE to not log using the method indicated by the
   name of the constant.  Options include to standard out (xLogToStdout), to a disk
   file (xLogToFile), and to a UDP port (xLogToUDP).  If xLogToUDP is set to pdTRUE
   then UDP messages are sent to the IP address configured as the echo server
   address (see the configECHO_SERVER_ADDR0 definitions in FreeRTOSConfig.h) and
   the port number set by configPRINT_PORT in FreeRTOSConfig.h. */
const BaseType_t xLogToStdout = pdTRUE, xLogToFile = pdFALSE, xLogToUDP = pdFALSE;

/* Default MAC address configuration.  The demo creates a virtual network
   connection that uses this MAC address by accessing the raw Ethernet data
   to and from a real network connection on the host PC.  See the
   configNETWORK_INTERFACE_TO_USE definition for information on how to configure
   the real network connection to use. */
const uint8_t ucMACAddress[6] = {configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5};
/*-----------------------------------------------------------*/

static UBaseType_t cheri_exception_handler(uintptr_t *exception_frame)
{
#ifdef __CHERI_PURE_CAPABILITY__
  size_t cause = 0;
  size_t epc = 0;
  size_t cheri_cause;

  asm volatile ("csrr %0, mcause" : "=r"(cause)::);
  asm volatile ("csrr %0, mepc" : "=r"(epc)::);

  size_t ccsr = 0;
  asm volatile ("csrr %0, mccsr" : "=r"(ccsr)::);

  uint8_t reg_num = (uint8_t) ((ccsr >> 10) & 0x1f);
  bool is_scr = ((ccsr >> 15) & 0x1);
  cheri_cause = (unsigned) ((ccsr >> 5) & 0x1f);

  for(int i = 0; i < 35; i++) {
    printf("x%i ", i); cheri_print_cap(*(exception_frame + i));
  }

  printf("mepc = 0x%lx\n", epc);
  printf("TRAP: CCSR = 0x%lx (cause: %x reg: %u : scr: %u)\n",
               ccsr,
               cheri_cause,
               reg_num, is_scr);
#endif
  while(1);
}

static UBaseType_t default_exception_handler(uintptr_t *exception_frame)
{
  size_t cause = 0;
  size_t epc = 0;
  asm volatile ("csrr %0, mcause" : "=r"(cause)::);
  asm volatile ("csrr %0, mepc" : "=r"(epc)::);
  printf("mcause = %u\n", cause);
  printf("mepc = %llx\n", epc);
  while(1);
}
/*-----------------------------------------------------------*/
void main_peekpoke(void);
static void prvServerWorkTask( void *pvParameters );

/*-----------------------------------------------------------*/

#define mainTCP_SERVER_STACK_SIZE               ( configMINIMAL_STACK_SIZE * 8 )

/* Handle of the task that runs the FTP and HTTP servers. */
static TaskHandle_t xServerWorkTaskHandle = NULL;

/* The server that manages the FTP and HTTP connections. */
TCPServer_t *pxTCPServer = NULL;

void main_peekpoke(void)
{
	/* Miscellaneous initialisation including preparing the logging and seeding
	   the random number generator. */
	prvMiscInitialisation();
	FreeRTOS_debug_printf(("mainn_peekpoke\r\n"));

	/* Initialise the network interface.
   ***NOTE*** Tasks that use the network are created in the network event hook
   when the network is connected and ready for use (see the definition of
   vApplicationIPNetworkEventHook() below).  The address values passed in here
   are used if ipconfigUSE_DHCP is set to 0, or if ipconfigUSE_DHCP is set to 1
   but a DHCP server cannot be	contacted. */
	FreeRTOS_debug_printf(("FreeRTOS_IPInit\r\n"));
	FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);

	/* Create the task that handles the FTP and HTTP servers.  This will
	   initialise the file system then wait for a notification from the network
	   event hook before creating the servers.  The task is created at the idle
	   priority, and sets itself to mainTCP_SERVER_TASK_PRIORITY after the file
	   system has initialised. */
	FreeRTOS_debug_printf(("xTaskCreate\r\n"));
	xTaskCreate( prvServerWorkTask, "SvrWork", mainTCP_SERVER_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xServerWorkTaskHandle );
}
/*-----------------------------------------------------------*/
static void prvServerWorkTask( void *pvParameters )
{
	/* TCPServer_t *pxTCPServer = NULL; */
	const TickType_t xInitialBlockTime = pdMS_TO_TICKS( 5000UL );

	/* A structure that defines the servers to be created.  Which servers are
	   included in the structure depends on the mainCREATE_HTTP_SERVER and
	   mainCREATE_FTP_SERVER settings at the top of this file. */
	static const struct xSERVER_CONFIG xServerConfiguration[] =
		{
#if( mainCREATE_HTTP_SERVER == 1 )
			/* Server type,		port number,	backlog, 	root dir. */
			{ eSERVER_HTTP, 	80, 			12, 		configHTTP_ROOT },
#endif

#if( mainCREATE_FTP_SERVER == 1 )
			/* Server type,		port number,	backlog, 	root dir. */
			{ eSERVER_FTP,  	21, 			12, 		"" }
#endif
		};


	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

	/* Can't serve web pages or start an FTP session until the card is
	   inserted.  The Xplained Pro hardware cannot generate an interrupt when
	   the card is inserted - so periodically look for the card until it is
	   inserted. */

	/* Not supported right now -- dwallach */
	/*
	  while( ( pxDisk = FF_SDDiskInit( mainSD_CARD_DISK_NAME ) ) == NULL )
	  {
	  vTaskDelay( xSDCardInsertDelay );
	  }

	  FreeRTOS_printf( ( "SD card detected, creating example files." ) );
	  prvCreateExampleFiles();
	  FreeRTOS_printf( ( "Example files created." ) );
	*/

	/* A disk is going to be created, so register the example file CLI
	   commands (which are very basic). */
	/* Not supported right now -- dwallach */
	/*
	  vRegisterFileSystemCLICommands();
	*/

	FreeRTOS_debug_printf(("prvServerWorkTask\r\n"));

	/* The priority of this task can be raised now the disk has been
	   initialised. */
	vTaskPrioritySet( NULL, mainECHO_SERVER_TASK_PRIORITY );

	/* Wait until the network is up before creating the servers.  The
	   notification is given from the network event hook. */
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

	/* Create the servers defined by the xServerConfiguration array above. */
	FreeRTOS_debug_printf(("Making TCP server\r\n"));
	pxTCPServer = FreeRTOS_CreateTCPServer( xServerConfiguration, sizeof( xServerConfiguration ) / sizeof( xServerConfiguration[ 0 ] ) );
	configASSERT( pxTCPServer );

	for( ;; )
	{
		/* The Xplained pro board does not define a pin for detecting the
		   removal of the SD card, so periodically check for the cards
		   presence. */
		// prvSDCardDetect();

		/* Run the HTTP and/or FTP servers, as configured above. */
		FreeRTOS_TCPServerWork( pxTCPServer, xInitialBlockTime );
	}
}

/* Called by FreeRTOS+TCP when the network connects or disconnects.  Disconnect
   events are only received if implemented in the MAC driver. */
void vApplicationIPNetworkEventHook(eIPCallbackEvent_t eNetworkEvent)
{
	uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
	char cBuffer[16];
	static BaseType_t xTasksAlreadyCreated = pdFALSE;

	/* If the network has just come up...*/
	if (eNetworkEvent == eNetworkUp)
	{
		FreeRTOS_debug_printf(("Network is up\r\n"));
		/* Create the tasks that use the IP stack if they have not already been
		   created. */
		if (xTasksAlreadyCreated == pdFALSE)
		{
			/* See the comments above the definitions of these pre-processor
			   macros at the top of this file for a description of the individual
			   demo tasks. */
			/* Let the server work task now it can now create the servers. */
			if( xServerWorkTaskHandle != NULL )
			{
				xTaskNotifyGive( xServerWorkTaskHandle );
			}

#if (mainCREATE_SIMPLE_UDP_CLIENT_SERVER_TASKS == 1)
			{
				printf("\r\n");
				printf("mainCREATE_SIMPLE_UDP_CLIENT_SERVER_TASKS=1\r\n");
				vStartSimpleUDPClientServerTasks(mainSIMPLE_UDP_CLIENT_SERVER_STACK_SIZE, mainSIMPLE_UDP_CLIENT_SERVER_PORT, mainSIMPLE_UDP_CLIENT_SERVER_TASK_PRIORITY);
			}
#endif /* mainCREATE_SIMPLE_UDP_CLIENT_SERVER_TASKS */

#if (mainCREATE_TCP_ECHO_TASKS_SINGLE == 1)
			{
				printf("\r\n");
				printf("mainCREATE_TCP_ECHO_TASKS_SINGLE=1\r\n");
				vStartTCPEchoClientTasks_SingleTasks(mainECHO_CLIENT_TASK_STACK_SIZE, mainECHO_CLIENT_TASK_PRIORITY);
			}
#endif /* mainCREATE_TCP_ECHO_TASKS_SINGLE */

#if (mainCREATE_TCP_ECHO_SERVER_TASK == 1)
			{
				printf("\r\n");
				printf("mainCREATE_TCP_ECHO_SERVER_TASK=1\r\n");
				vStartSimpleTCPServerTasks(mainECHO_SERVER_TASK_STACK_SIZE, mainECHO_SERVER_TASK_PRIORITY);
			}
#endif

			xTasksAlreadyCreated = pdTRUE;
		}

		/* Print out the network configuration, which may have come from a DHCP
		   server. */
		FreeRTOS_GetAddressConfiguration(&ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress);
		FreeRTOS_inet_ntoa(ulIPAddress, cBuffer);
		FreeRTOS_printf(("\r\n\r\nIP Address: %s\r\n", cBuffer));

		FreeRTOS_inet_ntoa(ulNetMask, cBuffer);
		FreeRTOS_printf(("Subnet Mask: %s\r\n", cBuffer));

		FreeRTOS_inet_ntoa(ulGatewayAddress, cBuffer);
		FreeRTOS_printf(("Gateway Address: %s\r\n", cBuffer));

		FreeRTOS_inet_ntoa(ulDNSServerAddress, cBuffer);
		FreeRTOS_printf(("DNS Server Address: %s\r\n\r\n\r\n", cBuffer));
	}
}

/*-----------------------------------------------------------*/

static void prvSRand(UBaseType_t ulSeed)
{
	/* Utility function to seed the pseudo random number generator. */
	ulNextRand = ulSeed;
}
/*-----------------------------------------------------------*/

static void prvMiscInitialisation(void)
{
	uint32_t seed = 42;
	FreeRTOS_debug_printf(("Seed for randomiser: %lu\r\n", seed));
	prvSRand((uint32_t)seed);
	FreeRTOS_debug_printf(("Random numbers: %08lX %08lX %08lX %08lX\r\n", ipconfigRAND32(), ipconfigRAND32(), ipconfigRAND32(), ipconfigRAND32()));

	#ifdef __CHERI_PURE_CAPABILITY__
		/* Setup an exception handler for CHERI */
		vPortSetExceptionHandler(0x1c, cheri_exception_handler);
	#endif
}
/*-----------------------------------------------------------*/

#if (ipconfigUSE_LLMNR != 0) || (ipconfigUSE_NBNS != 0) || (ipconfigDHCP_REGISTER_HOSTNAME == 1)

const char *pcApplicationHostnameHook(void)
{
	/* Assign the name "FreeRTOS" to this network node.  This function will
	   be called during the DHCP: the machine will be registered with an IP
	   address plus this name. */
	return mainHOST_NAME;
}

#endif
/*-----------------------------------------------------------*/

#if (ipconfigUSE_LLMNR != 0) || (ipconfigUSE_NBNS != 0)

BaseType_t xApplicationDNSQueryHook(const char *pcName)
{
	BaseType_t xReturn;

	/* Determine if a name lookup is for this node.  Two names are given
	   to this node: that returned by pcApplicationHostnameHook() and that set
	   by mainDEVICE_NICK_NAME. */
	if (_stricmp(pcName, pcApplicationHostnameHook()) == 0)
	{
		xReturn = pdPASS;
	}
	else if (_stricmp(pcName, mainDEVICE_NICK_NAME) == 0)
	{
		xReturn = pdPASS;
	}
	else
	{
		xReturn = pdFAIL;
	}

	return xReturn;
}

#endif

/*
 * Callback that provides the inputs necessary to generate a randomized TCP
 * Initial Sequence Number per RFC 6528.  THIS IS ONLY A DUMMY IMPLEMENTATION
 * THAT RETURNS A PSEUDO RANDOM NUMBER SO IS NOT INTENDED FOR USE IN PRODUCTION
 * SYSTEMS.
 */
uint32_t ulApplicationGetNextSequenceNumber(uint32_t ulSourceAddress,
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

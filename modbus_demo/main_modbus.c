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

/*
 * Called by main when mainDEMO_TYPE is set to 42 in
 * main.c.
 */
void main_modbus(void);

/*-----------------------------------------------------------*/

/* The queue used to communicate requests from client to server. */
QueueHandle_t xQueueClientServer;

/* The queue used to communicate responses from server to client. */
QueueHandle_t xQueueServerClient;

#if defined(MACAROONS_LAYER)
/* The queue used to communicate Macaroons from client to server. */
QueueHandle_t xQueueClientServerMacaroons;

/* The queue used to communicate Macaroons from server to client. */
QueueHandle_t xQueueServerClientMacaroons;
#endif

/*-----------------------------------------------------------*/

void main_modbus(void)
{
  /* Create the request queue. Sized to hold one modbus_queue_msg_t* */
  xQueueClientServer = xQueueCreate(mainQUEUE_LENGTH, sizeof(modbus_queue_msg_t *));

  /* Create the process queue. Sized to hold one modbus_queue_msg_t* */
  xQueueServerClient = xQueueCreate(mainQUEUE_LENGTH, sizeof(modbus_queue_msg_t *));

#if defined(MACAROONS_LAYER)
  /* Create the request queue. Sized to hold a macaroons_queue_msg_t*, which is a serialised macaroon and it's length */
  xQueueClientServerMacaroons = xQueueCreate(mainQUEUE_LENGTH, sizeof(macaroons_queue_msg_t *));

  /* Create the process queue. Sized to hold a macaroons_queue_msg_t*, which is a serialised macaroon and it's length */
  xQueueServerClientMacaroons = xQueueCreate(mainQUEUE_LENGTH, sizeof(macaroons_queue_msg_t *));
#endif

  /* modbus connection variables */
  char *ip = "127.0.0.1";
  int port = 1052;

  if (xQueueClientServer == NULL || xQueueServerClient == NULL)
    _exit(0);

#if defined(MACAROONS_LAYER)
  if (xQueueClientServerMacaroons == NULL || xQueueServerClientMacaroons == NULL)
      _exit(0);
#endif


  /* Initialise the server and client */
  vServerInitialization(ip, port, xQueueClientServer, xQueueServerClient);
  vClientInitialization(ip, port, xQueueClientServer, xQueueServerClient);

  /*
    * Start the client and server tasks as described in the comments at the top of this
    * file.
    */
  xTaskCreate(vClientTask,                 /* The function that implements the task. */
              "Client",                      /* The text name assigned to the task - for debug only as it is not used by the kernel. */
              configMINIMAL_STACK_SIZE * 2U, /* The size of the stack to allocate to the task. */
              NULL,                          /* The parameter passed to the task - not used in this case. */
              mainCLIENT_TASK_PRIORITY,      /* The priority assigned to the task. */
              NULL);                         /* The task handle is not required, so NULL is passed. */

  xTaskCreate(vServerTask,                 /* The function that implements the task. */
              "Server",                      /* The text name assigned to the task - for debug only as it is not used by the kernel. */
              configMINIMAL_STACK_SIZE * 2U, /* The size of the stack to allocate to the task. */
              NULL,                          /* The parameter passed to the task - not used in this case. */
              mainSERVER_TASK_PRIORITY,      /* The priority assigned to the task. */
              NULL);                         /* The task handle is not required, so NULL is passed. */

  /* Start the tasks and timer running. */
  vTaskStartScheduler();

  /* If all is well, the scheduler will now be running, and the following
  line will never be reached.  If the following line does execute, then
  there was insufficient FreeRTOS heap memory available for the Idle and/or
  timer tasks to be created.  See the memory management section on the
  FreeRTOS web site for more details on the FreeRTOS heap
  http://www.freertos.org/a00111.html. */
  for (;;)
    ;
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

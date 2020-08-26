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
 */

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

/* Modbus CHERI includes */
#if defined(CHERI_LAYER)
#include "modbus_cheri.h"
#endif

/* Modbus Macaroons includes */
#if defined(MACAROONS_LAYER)
#include "modbus_macaroons.h"
#endif

/*-----------------------------------------------------------*/

#if defined(MICROBENCHMARK)
typedef struct _PrintTaskStatus_t {
    char pcModbusFunctionName[MODBUS_MAX_FUNCTION_NAME_LEN];
    char pcTaskName[configMAX_TASK_NAME_LEN];
    uint32_t ulRunTimeCounter;
    uint16_t usStackHighWaterMark;
    uint32_t ulTotalRunTime;
} PrintTaskStatus_t;
#endif

/*-----------------------------------------------------------*/

static void prvCriticalSectionTask(void *pvParameters);

static int prvCriticalSectionWrapper(const uint8_t *req, const int req_length,
        uint8_t *rsp, int *rsp_length);

#if defined(MICROBENCHMARK)
static void prvPrintStatsTask(void *pvParameters);

static int prvBuildPrintTaskStatusStruct(const char *pcModbusFunctionName,
        TaskStatus_t *pxTaskStatus,
        PrintTaskStatus_t *pxPrintTaskStatus,
        uint32_t *pulTotalRunTime);
#endif

/*-----------------------------------------------------------*/

/* The structure holding state information. */
static modbus_mapping_t *mb_mapping = NULL;

/* The structure holding connection information. */
static modbus_t *ctx = NULL;

/* The queue used to send requests to prvCriticalSectionTask */
QueueHandle_t xQueueRequest;

/* The queue used to send responses from prvCriticalSectionTask */
QueueHandle_t xQueueResponse;

#if defined(MICROBENCHMARK)
/* The queue used to send task stats to prvPrintStatsTask */
QueueHandle_t xQueuePrint;
#endif

#if defined(MACAROONS_LAYER)
/* The queue used to communicate Macaroons from client to server. */
extern QueueHandle_t xQueueClientServerMacaroons;

/* The queue used to communicate Macaroons from server to client. */
extern QueueHandle_t xQueueServerClientMacaroons;
#endif

/*-----------------------------------------------------------*/

void vServerInitialization(char *ip, int port,
                             QueueHandle_t xQueueClientServer,
                             QueueHandle_t xQueueServerClient)
{
  /* initialise the connection */
  ctx = modbus_new_tcp(ip, port);
  if (ctx == NULL)
  {
    fprintf(stderr, "Failed to allocate ctx: %s\n",
            modbus_strerror(errno));
    modbus_free(ctx);
    _exit(0);
  }

#ifdef NDEBUG
  modbus_set_debug(ctx, pdFALSE);
#else
  modbus_set_debug(ctx, pdTRUE);
#endif

  /* initialise modbus queues and set the server state */
  modbus_set_request_queue(ctx, xQueueClientServer);
  modbus_set_response_queue(ctx, xQueueServerClient);
  modbus_set_server(ctx, pdTRUE);

/* initialise state (mb_mapping) */
#if defined(CHERI_LAYER) && !defined(CHERI_LAYER_STUBS)
  mb_mapping = modbus_mapping_new_start_address_cheri(
      ctx,
      UT_BITS_ADDRESS, UT_BITS_NB,
      UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB,
      UT_REGISTERS_ADDRESS, UT_REGISTERS_NB_MAX,
      UT_INPUT_REGISTERS_ADDRESS, UT_INPUT_REGISTERS_NB);
#else
  mb_mapping = modbus_mapping_new_start_address(
      UT_BITS_ADDRESS, UT_BITS_NB,
      UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB,
      UT_REGISTERS_ADDRESS, UT_REGISTERS_NB_MAX,
      UT_INPUT_REGISTERS_ADDRESS, UT_INPUT_REGISTERS_NB);
#endif

  /* check for successful initialisation of state */
  if (mb_mapping == NULL)
  {
    fprintf(stderr, "Failed to allocate the mapping: %s\n",
            modbus_strerror(errno));
    modbus_free(ctx);
    _exit(0);
  }

  /* display the state if DEBUG */
  if (modbus_get_debug(ctx))
  {
    print_mb_mapping(mb_mapping);
  }

  /* Initialize coils */
  modbus_set_bits_from_bytes(mb_mapping->tab_input_bits, 0, UT_INPUT_BITS_NB,
                             UT_INPUT_BITS_TAB);

  /* Initialize discrete inputs */
  for (int i = 0; i < UT_INPUT_REGISTERS_NB; i++)
  {
    mb_mapping->tab_input_registers[i] = UT_INPUT_REGISTERS_TAB[i];
  }

#if defined(MACAROONS_LAYER)
  /* Initialise Macaroon */
  int rc;
  char *key = "a bad secret";
  char *id = "id for a bad secret";
  char *location = "https://www.modbus.com/macaroons/";
  rc = initialise_server_macaroon(ctx, location, key, id);
  configASSERT(rc != -1);

  /**
   * serialise the macaroon and queue it for the client
   *
   * this is a TOFU operation. the first client to dequeue it gets it...
   * */
  rc = queue_server_macaroon(ctx, xQueueServerClientMacaroons);
  configASSERT(rc != -1);
#endif

  /* Initialise queues for comms with prvCriticalSectionTask */
  xQueueRequest = xQueueCreate(mainQUEUE_LENGTH, sizeof(modbus_queue_msg_t *));
  xQueueResponse = xQueueCreate(mainQUEUE_LENGTH, sizeof(modbus_queue_msg_t *));

}

/*-----------------------------------------------------------*/

void vServerTask(void *pvParameters)
{
#ifndef NDEBUG
  print_shim_info("modbus_server", __FUNCTION__);
#endif

  int rc;
  BaseType_t xReturned;
  char *pcModbusFunctionName;

  /* Remove compiler warning about unused parameter. */
  (void)pvParameters;

  /* variables for comms with libmodbus */
  uint8_t *req = (uint8_t *)pvPortMalloc(MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));
  int req_length = 0;
  uint8_t *rsp = (uint8_t *)pvPortMalloc(MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));
  int rsp_length = 0;

  /* variables for comms with prvCriticalSectionTask */
  modbus_queue_msg_t *pxQueueReq = (modbus_queue_msg_t *)pvPortMalloc(sizeof(modbus_queue_msg_t));
  modbus_queue_msg_t *pxQueueRsp;

  /* Create the task prvCriticalSectionTask */
  xTaskCreate( prvCriticalSectionTask,
          "critical",
          configMINIMAL_STACK_SIZE * 2U,
          NULL,
          prvCRITICAL_SECTION_TASK_PRIORITY,
          NULL);

#if defined(MICROBENCHMARK)
  /* Create the task prvPrintStatsTask */
  xTaskCreate( prvPrintStatsTask,
          "print",
          configMINIMAL_STACK_SIZE * 2U,
          NULL,
          prvPRINT_STATS_TASK_PRIORITY,
          NULL);
#endif

#if defined(MICROBENCHMARK)
  /* variables for microbenchmarking
   *
   * NOTE: using uxTaskGetNumberOfTasks() must occur AFTER creating the two tasks above */
  UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
  TaskStatus_t *pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
  PrintTaskStatus_t xPrintTaskStatus;
  UBaseType_t uxArraySizeReturned = 0;
  uint32_t *pulTotalRunTime = (uint32_t *)pvPortMalloc(sizeof(uint32_t));

  /* Initialise queue for comms with prvPrintStatsTask
   *
   * the queue needs to be able to hold a TaskStatus_t structure for all tasks for
   * all the iterations for a given function, because we're only guaranteed idle time
   * when we return to the client, which blocks for 200ms between commands.  */
  xQueuePrint = xQueueCreate(mainMICROBENCHMARK_ITERATIONS * uxArraySize, sizeof(PrintTaskStatus_t));
#endif

  for (;;)
  {
    /* Wait until something arrives in the queue - this task will block
    indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
    FreeRTOSConfig.h. */
    req_length = modbus_receive(ctx, req);

#ifndef NDEBUG
    print_shim_info("modbus_server", __FUNCTION__);
    printf("SERVER:\tReceived request\r\n");
#endif

    /* get the modbus function name from the request */
    pcModbusFunctionName = modbus_get_function_name(ctx, req);

#if defined(MICROBENCHMARK)
    /* discard mainMICROBENCHMARK_DISCARD runs to ensure quiescence */
    for (int i = 0; i < mainMICROBENCHMARK_DISCARD; ++i)
    {
        rc = prvCriticalSectionWrapper(req, req_length, rsp, &rsp_length);
        configASSERT(rc != -1);

        uxArraySizeReturned = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize,
                pulTotalRunTime);

        configASSERT(uxArraySizeReturned == uxArraySize);

    }

    /* perform mainMICROBENCHMARK_ITERATIONS runs of the critical section */
    for (int i = 0; i < mainMICROBENCHMARK_ITERATIONS; ++i)
    {
        rc = prvCriticalSectionWrapper(req, req_length, rsp, &rsp_length);
        configASSERT(rc != -1);

        uxArraySizeReturned = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize,
                pulTotalRunTime);

        configASSERT(uxArraySizeReturned == uxArraySize);

        for (int j = 0; j < uxArraySize; ++j)
        {
            rc = prvBuildPrintTaskStatusStruct(pcModbusFunctionName, &pxTaskStatusArray[j],
                    &xPrintTaskStatus, pulTotalRunTime);
            configASSERT(rc != -1);

            xReturned = xQueueSend(xQueuePrint, &xPrintTaskStatus, 0U);
            configASSERT(xReturned == pdPASS);
        }
    }
#else
    rc = prvCriticalSectionWrapper(req, req_length, rsp, &rsp_length);
    configASSERT(rc != -1);
#endif

#ifndef NDEBUG
    print_shim_info("modbus_server", __FUNCTION__);
    printf("SERVER: Sending response\r\n");
#endif

    rc = modbus_reply(ctx, rsp, rsp_length);
    configASSERT(rc != -1);
  }
}

/*-----------------------------------------------------------*/

static int prvCriticalSectionWrapper(const uint8_t *req, const int req_length,
        uint8_t *rsp, int *rsp_length)
{
    BaseType_t xReturned;
    modbus_queue_msg_t *pxQueueReq = (modbus_queue_msg_t *)pvPortMalloc(sizeof(modbus_queue_msg_t));
    modbus_queue_msg_t *pxQueueRsp;

    /* queue the request
     *
     * prvCriticalSectionTask will preemt and process the request
     * since it has a higher priority */
    pxQueueReq->msg = req;
    pxQueueReq->msg_length = req_length;
    xReturned = xQueueSend(xQueueRequest, &pxQueueReq, 0U);
    configASSERT(xReturned == pdPASS);

    /* dequeue the response and extract req and req_length */
    xQueueReceive(xQueueResponse, &pxQueueRsp, portMAX_DELAY);
    *rsp_length = pxQueueRsp->msg_length;
    memcpy(rsp, pxQueueRsp->msg, *rsp_length);

    vPortFree(pxQueueReq);

    return 0;
}

/*-----------------------------------------------------------*/

static void prvCriticalSectionTask(void *pvParameters)
{
    int rc;
    BaseType_t xReturned;

    modbus_queue_msg_t *pxQueueReq;
    uint8_t *req;
    int req_length = 0;

    modbus_queue_msg_t *pxQueueRsp = (modbus_queue_msg_t *)pvPortMalloc(sizeof(modbus_queue_msg_t));
    uint8_t *rsp = (uint8_t *)pvPortMalloc(MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));
    int rsp_length = 0;

    for (;;)
    {
        /* dequeue a request */
        xQueueReceive(xQueueRequest, &pxQueueReq, portMAX_DELAY);
        req = pxQueueReq->msg;
        req_length = pxQueueReq->msg_length;

        /* Ensure access to mb_mapping and ctx cannot be interrupted while processing
         * a request from the client */
        taskENTER_CRITICAL();

        /**
         * if CHERI then perform the CHERI layer of preprocessing of the state
         * if MACAROONS then perform the Macaroons layer of preprocessing of the state
         * then perform the normal processing
         * NB order matters here:
         * - First reduce permissions on state
         * - Then verify Macaroons, if appropriate
         * - Then perform the normal processing
         * NB The Basic configuration, which bypasses both of these layers can still
         * be compiled for a CHERI system, it just wont restrict the state before
         * processing the request.
         * */
#if defined(CHERI_LAYER_STUBS)
        /* this is only used to evaluate the overhead of calling a function */
        rc = modbus_preprocess_request_cheri_baseline(ctx, req, mb_mapping);
        configASSERT(rc != -1);
#elif defined(CHERI_LAYER)
        rc = modbus_preprocess_request_cheri(ctx, req, mb_mapping);
        configASSERT(rc != -1);
#endif

#if defined(MACAROONS_LAYER)
        rc = modbus_preprocess_request_macaroons(ctx, req);
        configASSERT(rc != -1);
#endif

        rc = modbus_process_request(ctx, req, req_length,
                rsp, &rsp_length, mb_mapping);

        /* critical access to mb_mapping and ctx is finished, so it's safe to exit */
        taskEXIT_CRITICAL();

        /* queue the response to send back to vServerTask */
        pxQueueRsp->msg = rsp;
        pxQueueRsp->msg_length = rsp_length;
        xReturned = xQueueSend(xQueueResponse, &pxQueueRsp, 0U);
        configASSERT(xReturned == pdPASS);
    }
}

/*-----------------------------------------------------------*/

#if defined(MICROBENCHMARK)
static void prvPrintStatsTask(void *pvParameters)
{
    PrintTaskStatus_t xPrintTaskStatus;
	/* Print out column headings for the run-time stats table. */
    printf("modbus_function_name, task_name, task_runtime_counter, stack_highwater_mark, total_runtime\n");
    for(;;)
    {
        /* dequeue PrintTaskStatus_t structures */
        xQueueReceive(xQueuePrint, &xPrintTaskStatus, portMAX_DELAY);
        printf("%s, %s, %u, %u, %u\n",
                xPrintTaskStatus.pcModbusFunctionName,
                xPrintTaskStatus.pcTaskName,
                xPrintTaskStatus.ulRunTimeCounter,
                xPrintTaskStatus.usStackHighWaterMark,
                xPrintTaskStatus.ulTotalRunTime);
    }
}
#endif

/*-----------------------------------------------------------*/

#if defined(MICROBENCHMARK)
static int prvBuildPrintTaskStatusStruct(const char *pcModbusFunctionName,
        TaskStatus_t *pxTaskStatus,
        PrintTaskStatus_t *pxPrintTaskStatus,
        uint32_t *pulTotalRunTime)
{
    if(pcModbusFunctionName == NULL ||
            pxTaskStatus == NULL ||
            pxPrintTaskStatus == NULL ||
            pulTotalRunTime == NULL)
    {
        return -1;
    }

    /* assign the pcModbusFunctionName */
    size_t xModbusFunctionNameLen = strnlen(pcModbusFunctionName, MODBUS_MAX_FUNCTION_NAME_LEN);
    strncpy(pxPrintTaskStatus->pcModbusFunctionName, pcModbusFunctionName, xModbusFunctionNameLen + 1);

    /* assign the task name */
    size_t xTaskNameLen = strnlen(pxTaskStatus->pcTaskName, configMAX_TASK_NAME_LEN);
    strncpy(pxPrintTaskStatus->pcTaskName, pxTaskStatus->pcTaskName, xTaskNameLen + 1);

    /* assign remaining elements of the PrintTaskStatus_t struct */
    pxPrintTaskStatus->ulRunTimeCounter = pxTaskStatus->ulRunTimeCounter;
    pxPrintTaskStatus->usStackHighWaterMark = pxTaskStatus->usStackHighWaterMark;
    pxPrintTaskStatus->ulTotalRunTime = *pulTotalRunTime;

    return 0;
}
#endif

/*-----------------------------------------------------------*/

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

/* Microbenchmark includes */
#include "microbenchmark.h"

/* type definitions */

typedef struct _PrintTaskStatus_t {
    char pcFunctionName[MODBUS_MAX_FUNCTION_NAME_LEN];
    char pcTaskName[configMAX_TASK_NAME_LEN];
    uint32_t ulRunTimeCounter;
    uint16_t usStackHighWaterMark;
    uint32_t ulTotalRunTime;
} PrintTaskStatus_t;

/* function declarations */

static BaseType_t prvBuildPrintTaskStatusStruct(const char *pcFunctionName,
        TaskStatus_t *pxTaskStatus,
        PrintTaskStatus_t *pxPrintTaskStatus,
        uint32_t *pulTotalRunTime);

/* static variable declarations */

/* The buffer holding PrintTaskStatus_t structs */
static PrintTaskStatus_t *pxPrintBuffer = NULL;
static size_t xPrintBufferSize = 0;
static size_t xPrintBufferCount = 0;

/*-----------------------------------------------------------*/

void xMicrobenchmarkSample(char *pcFunctionName, BaseType_t xToPrint)
{
    UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
    TaskStatus_t *pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    UBaseType_t uxArraySizeReturned = 0;
    uint32_t *pulTotalRunTime = (uint32_t *)pvPortMalloc(sizeof(uint32_t));
    BaseType_t xReturned;

    /* initialise the buffer, if necessary */
    if (pxPrintBuffer == NULL)
    {
        xPrintBufferSize = MAX_FUNCTIONS * MICROBENCHMARK_ITERATIONS * uxArraySize;
        pxPrintBuffer = (PrintTaskStatus_t *)pvPortMalloc(
                xPrintBufferSize * sizeof(PrintTaskStatus_t));
        configASSERT(pxPrintBuffer != NULL);
    }

    /* sample all task states */
    uxArraySizeReturned = uxTaskGetSystemState(pxTaskStatusArray,
            uxArraySize, pulTotalRunTime);
    configASSERT(uxArraySizeReturned == uxArraySize);

    if(xToPrint)
    {
        /* build PrintTastStatusStruct and add to the print buffer */
        for (UBaseType_t j = 0; j < uxArraySize; ++j)
        {
            xReturned = prvBuildPrintTaskStatusStruct(pcFunctionName, &pxTaskStatusArray[j],
                    &pxPrintBuffer[xPrintBufferCount], pulTotalRunTime);
            configASSERT(xReturned == pdPASS);
            xPrintBufferCount += 1;
        }
    }
    vPortFree(pxTaskStatusArray);
    vPortFree(pulTotalRunTime);
}

/*-----------------------------------------------------------*/

static BaseType_t prvBuildPrintTaskStatusStruct(const char *pcFunctionName,
        TaskStatus_t *pxTaskStatus,
        PrintTaskStatus_t *pxPrintTaskStatus,
        uint32_t *pulTotalRunTime)
{
    if(pcFunctionName == NULL ||
            pxTaskStatus == NULL ||
            pxPrintTaskStatus == NULL ||
            pulTotalRunTime == NULL)
    {
        return pdFAIL;
    }

    /* assign the pcFunctionName */
    size_t xFunctionNameLen = strnlen(pcFunctionName, MODBUS_MAX_FUNCTION_NAME_LEN);
    strncpy(pxPrintTaskStatus->pcFunctionName, pcFunctionName, xFunctionNameLen + 1);

    /* assign the task name */
    size_t xTaskNameLen = strnlen(pxTaskStatus->pcTaskName, configMAX_TASK_NAME_LEN);
    strncpy(pxPrintTaskStatus->pcTaskName, pxTaskStatus->pcTaskName, xTaskNameLen + 1);

    /* assign remaining elements of the PrintTaskStatus_t struct */
    pxPrintTaskStatus->ulRunTimeCounter = pxTaskStatus->ulRunTimeCounter;
    pxPrintTaskStatus->usStackHighWaterMark = pxTaskStatus->usStackHighWaterMark;
    pxPrintTaskStatus->ulTotalRunTime = *pulTotalRunTime;

    return pdPASS;
}

/*-----------------------------------------------------------*/

void vPrintMicrobenchmarkSamples(void)
{
	/* Print out column headings for the run-time stats table. */
    printf("modbus_function_name, task_name, task_runtime_counter, stack_highwater_mark, total_runtime\n");
    for(int i = 0; i < xPrintBufferCount; ++i)
    {
        printf("%s, %s, %u, %u, %u\n",
                pxPrintBuffer[i].pcFunctionName,
                pxPrintBuffer[i].pcTaskName,
                pxPrintBuffer[i].ulRunTimeCounter,
                pxPrintBuffer[i].usStackHighWaterMark,
                pxPrintBuffer[i].ulTotalRunTime);
    }
}

/*-----------------------------------------------------------*/


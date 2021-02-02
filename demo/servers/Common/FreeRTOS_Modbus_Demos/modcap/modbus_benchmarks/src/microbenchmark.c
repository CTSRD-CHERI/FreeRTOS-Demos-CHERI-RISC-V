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
#include <assert.h>

#if defined(__freertos__)
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#endif

/* Modbus includes. */
#include <modbus/modbus.h>
#include <modbus/modbus-helpers.h>

/* Microbenchmark includes */
#include "microbenchmark.h"

/* type definitions */

typedef struct _BenchmarkSample_t {
    BenchmarkType_t xBenchmark;
    char pcFunctionName[ MODBUS_MAX_FUNCTION_NAME_LEN ];
    uint32_t ulTimeDiff;
} BenchmarkSample_t;

/* static variable declarations */

/* The buffer holding PrintTaskStatus_t structs */
static BenchmarkSample_t *pxPrintBuffer = NULL;
static size_t xPrintBufferSize = 0;
static size_t xPrintBufferCount = 0;

/*-----------------------------------------------------------*/
#if defined(__freertos__)
void xMicrobenchmarkSample( BenchmarkType_t xBenchmark, char *pcFunctionName,
        uint64_t ulTimeDiff, BaseType_t xToPrint )
#else
void xMicrobenchmarkSample( BenchmarkType_t xBenchmark, char *pcFunctionName,
        uint64_t ulTimeDiff, uint8_t xToPrint )
#endif
{
    size_t xFunctionNameLen = strnlen(pcFunctionName, MODBUS_MAX_FUNCTION_NAME_LEN);

    /* initialise the buffer, if necessary */
    if( pxPrintBuffer == NULL )
    {
        /* Set the initial buffer size, assuming we will iterate
         * through each function 10 times. */
        xPrintBufferSize = MAX_FUNCTIONS * 10;
        xPrintBufferCount = 0;
#if defined(__freertos__)
        pxPrintBuffer = ( BenchmarkSample_t * )pvPortMalloc(
                xPrintBufferSize * sizeof( BenchmarkSample_t ) );
        configASSERT( pxPrintBuffer != NULL );
#else
        pxPrintBuffer = ( BenchmarkSample_t * )malloc(
                xPrintBufferSize * sizeof( BenchmarkSample_t ) );
        assert( pxPrintBuffer != NULL );
#endif
    }

    if( xToPrint )
    {
        /* Check if there is room in the print buffer.  If not, double it's allocation.
         * There's no FreeRTOS_realloc(), so we basically need to create a new buffer, copy
         * everything in from the previous allocation, and then deallocate the original. */
        if( xPrintBufferCount == xPrintBufferSize)
        {
            /* Create the new allocation. */
#if defined(__freertos__)
            BenchmarkSample_t *pxPrintBufferNew = ( BenchmarkSample_t * )pvPortMalloc(
                ( xPrintBufferSize * 2 ) * sizeof( BenchmarkSample_t ) );
#else
            BenchmarkSample_t *pxPrintBufferNew = ( BenchmarkSample_t * )malloc(
                ( xPrintBufferSize * 2 ) * sizeof( BenchmarkSample_t ) );
#endif

            /* Copy in structures from the original allocation. */
            for( int i = 0; i < xPrintBufferCount; ++i )
            {
                pxPrintBufferNew[ i ] = pxPrintBuffer[ i ];
            }

            /* Free the original allocation. */
#if defined(__freertos__)
            vPortFree( pxPrintBuffer );
#else
            free( pxPrintBuffer );
#endif

            /* Reassign the pointer. */
            pxPrintBuffer = pxPrintBufferNew;

            xPrintBufferSize = xPrintBufferSize * 2;
        }
#if defined(__freertos__)
        configASSERT ( xPrintBufferCount < xPrintBufferSize);
#else
        assert ( xPrintBufferCount < xPrintBufferSize);
#endif

        /* populate BenchmarkSample_t struct and add to the print buffer */
        pxPrintBuffer[ xPrintBufferCount ].xBenchmark = xBenchmark;
        pxPrintBuffer[ xPrintBufferCount ].ulTimeDiff = ulTimeDiff;
        strncpy ( pxPrintBuffer[ xPrintBufferCount ].pcFunctionName, pcFunctionName, xFunctionNameLen + 1 );
        xPrintBufferCount += 1;
    }
}

/*-----------------------------------------------------------*/
void vPrintMicrobenchmarkSamples( void )
{
    char *spare_string = "SPARE_PROCESSING_MICROBENCHMARK";
    char *request_string = "REQUEST_PROCESSING_MICROBENCHMARK";
    char *max_string = "MAX_PROCESSING_MACROBENCHMARK";
    char *print_string;

    /* Print out column headings for the run-time stats table. */
    printf( "benchmark_type, modbus_function_name, time_diff\n" );
    for( int i = 0; i < xPrintBufferCount; ++i)
    {
        if( pxPrintBuffer[ i ].xBenchmark == SPARE_PROCESSING )
        {
            print_string = spare_string;
        }
        else if( pxPrintBuffer[ i ].xBenchmark == REQUEST_PROCESSING )
        {
            print_string = request_string;
        }
        else
        {
            print_string = max_string;
        }

        printf( "%s, %s, %u\n",
                print_string,
                pxPrintBuffer[ i ].pcFunctionName,
                pxPrintBuffer[ i ].ulTimeDiff );
    }

    /* Reset the buffer. */
#if defined(__freertos__)
    vPortFree(pxPrintBuffer);
#else
    free(pxPrintBuffer);
#endif
    pxPrintBuffer = NULL;
}

/*-----------------------------------------------------------*/


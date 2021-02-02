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

#ifndef _MODBUS_BENCHMARKS_H_
#define _MODBUS_BENCHMARKS_H_

/*-----------------------------------------------------------*/

/* bound the number of functions that will be benchmarked */
#define MAX_FUNCTIONS 20

/*-----------------------------------------------------------*/

/*
 * SPARE_PREOCESSING: @ server. Measures spare time after processing a request.
 * REQUEST_PROCESSING: @ server. Measures time to process a request.
 * MAX_PROCESSING: @ client. Measures time for request/reply roundtrips.
 */
typedef enum _BenchmarkType_t {
    SPARE_PROCESSING,
    REQUEST_PROCESSING,
    MAX_PROCESSING
} BenchmarkType_t;

/*-----------------------------------------------------------*/

#if defined(__freertos__)
void xMicrobenchmarkSample( BenchmarkType_t xBenchmark, char *pcFunctionName,
        uint64_t ulTimeDiff, BaseType_t xToPrint );
#else
void xMicrobenchmarkSample( BenchmarkType_t xBenchmark, char *pcFunctionName,
        uint64_t ulTimeDiff, uint8_t xToPrint );
#endif

void vPrintMicrobenchmarkSamples(void);

/*-----------------------------------------------------------*/

#endif /* _MODBUS_BENCHMARKS_H_ */

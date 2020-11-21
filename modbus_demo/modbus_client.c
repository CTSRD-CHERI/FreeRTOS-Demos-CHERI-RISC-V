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
#include "modbus_client.h"

/* Modbus Macaroons includes */
#if defined(MACAROONS_LAYER)
#include "modbus_macaroons.h"
#endif

/* Microbenchmark includes */
#if defined(MICROBENCHMARK)
#include "microbenchmark.h"
#endif

/*-----------------------------------------------------------*/

/* Closes the connection, frees memory, and deletes the task */
static void prvClientTaskShutdown(void);

/* Helper function for converting to libmodbus formats */
static int convert_string_req(const char *req_string, uint8_t *req);

/*-----------------------------------------------------------*/

/* libmodbus variables */
static modbus_t *ctx = NULL;
static uint8_t *tab_rp_bits = NULL;
static uint16_t *tab_rp_registers = NULL;
static uint8_t *tab_rp_string = NULL;

/*-----------------------------------------------------------*/

void vClientInitialization(char *ip, int port)
{
    /* initialise the connection */
    ctx = modbus_new_tcp(ip, port);
    if (ctx == NULL)
    {
        fprintf(stderr, "Failed to allocate ctx: %s\n",
                modbus_strerror(errno));
        prvClientTaskShutdown();
    }

#ifdef NDEBUG
    modbus_set_debug(ctx, pdFALSE);
#else
    modbus_set_debug(ctx, pdTRUE);
#endif
}

/*-----------------------------------------------------------*/

void vClientTask(void *pvParameters)
{
    FreeRTOS_debug_printf( ("Enter client task\n") );

    TickType_t xNextWakeTime;
    int rc;
    int nb_points;

    /* Remove compiler warning about unused parameter. */
    (void)pvParameters;

    /* Initialise xNextWakeTime - this only needs to be done once. */
    xNextWakeTime = xTaskGetTickCount();

    /* Connect to the modbus server */
    if(modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed\n");
        prvClientTaskShutdown();
    }

    /* Allocate and initialize the memory to store the bits */
    nb_points = (UT_BITS_NB > UT_INPUT_BITS_NB) ? UT_BITS_NB : UT_INPUT_BITS_NB;
    tab_rp_bits = (uint8_t *)pvPortMalloc(nb_points * sizeof(uint8_t));
    memset(tab_rp_bits, 0, nb_points * sizeof(uint8_t));

    /* Allocate and initialize the memory to store the registers */
    nb_points = (UT_REGISTERS_NB > UT_INPUT_REGISTERS_NB) ?
        UT_REGISTERS_NB : UT_INPUT_REGISTERS_NB;
    tab_rp_registers = (uint16_t *) pvPortMalloc(nb_points * sizeof(uint16_t));
    memset(tab_rp_registers, 0, nb_points * sizeof(uint16_t));

    /* Allocate and initialize the memory to store the string */
    tab_rp_string = (uint8_t *)pvPortMalloc(MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));
    int nb_chars = convert_string_req((const char *)TEST_STRING, tab_rp_string);

#if defined(MICROBENCHMARK)
    /* If we're benchmarking, the server will be iterating over the request,
     * so we bump up the time the client is willing to wait for a response */
    rc = modbus_set_response_timeout(ctx, 1000, 0);
#endif

    printf("BEGIN_TEST\n");

    /**************
     * STRING TESTS
     *************/

#if defined(MACAROONS_LAYER)
    /* when using macaroons, the server will initialise a macaroon and store
     * it in mb_mapping->tab_string, so the client starts by reading that
     * string and initialising it's own macaroon.
     *
     * this is basically a TOFU operation: we'll deserialise it and store
     * it as the client's initial macaroon */
    memset(tab_rp_string, 0, MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));
    rc = modbus_read_string(ctx, tab_rp_string);
    configASSERT(rc != -1);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    rc = initialise_client_macaroon(ctx, (char *)tab_rp_string, rc);
    configASSERT(rc != -1);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);
#endif

    /* WRITE_STRING */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nWRITE_STRING\n");
#endif

    rc = modbus_write_string(ctx, tab_rp_string, nb_chars);
    configASSERT(rc != -1);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* READ_STRING */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nREAD_STRING\n");
#endif

    memset(tab_rp_string, 0, MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));
    rc = modbus_read_string(ctx, tab_rp_string);
    configASSERT(rc != -1);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /************
     * COIL TESTS
     ***********/

    /* WRITE_SINGLE_COIL */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nWRITE_SINGLE_COIL\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_write_bit_macaroons(ctx, UT_BITS_ADDRESS, ON);
#else
    rc = modbus_write_bit(ctx, UT_BITS_ADDRESS, ON);
#endif
    configASSERT(rc == 1);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* READ_SINGLE_COIL */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nREAD_SINGLE_COIL\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_read_bits_macaroons(ctx, UT_BITS_ADDRESS, 1, tab_rp_bits);
#else
    rc = modbus_read_bits(ctx, UT_BITS_ADDRESS, 1, tab_rp_bits);
#endif
    configASSERT(rc == 1);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* WRITE_MULTIPLE_COILS */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nWRITE_MULTIPLE_COILS\n");
#endif
    {
        uint8_t tab_value[UT_BITS_NB];
        modbus_set_bits_from_bytes(tab_value, 0, UT_BITS_NB, UT_BITS_TAB);

#if defined(MACAROONS_LAYER)
        rc = modbus_write_bits_macaroons(ctx, UT_BITS_ADDRESS, UT_BITS_NB, tab_value);
#else
        rc = modbus_write_bits(ctx, UT_BITS_ADDRESS, UT_BITS_NB, tab_value);
#endif
    }
    configASSERT(rc == UT_BITS_NB);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* READ_MULTIPLE_COILS */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nREAD_MULTIPLE_COILS\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_read_bits_macaroons(ctx, UT_BITS_ADDRESS, UT_BITS_NB, tab_rp_bits);
#else
    rc = modbus_read_bits(ctx, UT_BITS_ADDRESS, UT_BITS_NB, tab_rp_bits);
#endif
    configASSERT(rc == UT_BITS_NB);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /**********************
     * DISCRETE INPUT TESTS
     *********************/

    /* READ_INPUT_BITS */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nREAD_INPUT_BITS\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_read_input_bits_macaroons(ctx, UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB, tab_rp_bits);
#else
    rc = modbus_read_input_bits(ctx, UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB, tab_rp_bits);
#endif
    configASSERT(rc == UT_INPUT_BITS_NB);

    {
        /* further checks on the returned discrete inputs */
        int idx = 0;
        uint8_t value;
        nb_points = UT_INPUT_BITS_NB;
        while (nb_points > 0) {
            int nb_bits = (nb_points > 8) ? 8 : nb_points;
            value = modbus_get_byte_from_bits(tab_rp_bits, idx*8, nb_bits);
            configASSERT(value == UT_INPUT_BITS_TAB[idx]);

            nb_points -= nb_bits;
            idx++;
        }
    }
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /************************
     * HOLDING REGISTER TESTS
     ***********************/

    /* WRITE_SINGLE_REGISTER */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nWRITE_SINGLE_REGISTER\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_write_register_macaroons(ctx, UT_REGISTERS_ADDRESS, 0x1234);
#else
    rc = modbus_write_register(ctx, UT_REGISTERS_ADDRESS, 0x1234);
#endif
    configASSERT(rc == 1);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* READ_SINGLE_REGISTER */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nREAD_SINGLE_REGISTER\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_read_registers_macaroons(ctx, UT_REGISTERS_ADDRESS, 1, tab_rp_registers);
#else
    rc = modbus_read_registers(ctx, UT_REGISTERS_ADDRESS, 1, tab_rp_registers);
#endif
    configASSERT(rc == 1);
    configASSERT(tab_rp_registers[0] == 0x1234);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* WRITE_MULTIPLE_REGISTERS */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nWRITE_MULTIPLE_REGISTERS\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_write_registers_macaroons(ctx, UT_REGISTERS_ADDRESS, UT_REGISTERS_NB, UT_REGISTERS_TAB);
#else
    rc = modbus_write_registers(ctx, UT_REGISTERS_ADDRESS, UT_REGISTERS_NB, UT_REGISTERS_TAB);
#endif
    configASSERT(rc == UT_REGISTERS_NB);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* READ_MULTIPLE_REGISTERS */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nREAD_MULTIPLE_REGISTERS\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_read_registers_macaroons(ctx, UT_REGISTERS_ADDRESS, UT_REGISTERS_NB, tab_rp_registers);
#else
    rc = modbus_read_registers(ctx, UT_REGISTERS_ADDRESS, UT_REGISTERS_NB, tab_rp_registers);
#endif
    configASSERT(rc == UT_REGISTERS_NB);
    for (int i=0; i < UT_REGISTERS_NB; i++) {
        configASSERT(tab_rp_registers[i] == UT_REGISTERS_TAB[i]);
    }
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* WRITE_AND_READ_REGISTERS */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nWRITE_AND_READ_REGISTERS\n");
#endif

    nb_points = (UT_REGISTERS_NB > UT_INPUT_REGISTERS_NB) ? UT_REGISTERS_NB : UT_INPUT_REGISTERS_NB;
    memset(tab_rp_registers, 0, nb_points * sizeof(uint16_t));

#if defined(MACAROONS_LAYER)
    rc = modbus_write_and_read_registers_macaroons(ctx,
            UT_REGISTERS_ADDRESS + 1,
            UT_REGISTERS_NB - 1,
            tab_rp_registers,
            UT_REGISTERS_ADDRESS,
            UT_REGISTERS_NB,
            tab_rp_registers);
#else
    rc = modbus_write_and_read_registers(ctx,
            UT_REGISTERS_ADDRESS + 1,
            UT_REGISTERS_NB - 1,
            tab_rp_registers,
            UT_REGISTERS_ADDRESS,
            UT_REGISTERS_NB,
            tab_rp_registers);
#endif
    configASSERT(rc == UT_REGISTERS_NB);
    configASSERT(tab_rp_registers[0] == UT_REGISTERS_TAB[0]);
    for (int i = 1; i < UT_REGISTERS_NB; i++)
    {
        configASSERT(tab_rp_registers[i] == 0);
    }
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /**********************
     * INPUT REGISTER TESTS
     *********************/

    /* READ_INPUT_REGISTERS */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nREAD_INPUT_REGISTERS\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_read_input_registers_macaroons(ctx, UT_INPUT_REGISTERS_ADDRESS,
            UT_INPUT_REGISTERS_NB, tab_rp_registers);
#else
    rc = modbus_read_input_registers(ctx, UT_INPUT_REGISTERS_ADDRESS,
            UT_INPUT_REGISTERS_NB, tab_rp_registers);
#endif
    configASSERT(rc == UT_INPUT_REGISTERS_NB);
    for (int i = 0; i < UT_INPUT_REGISTERS_NB; i++)
    {
        configASSERT(tab_rp_registers[i] == UT_INPUT_REGISTERS_TAB[i]);
    }
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /*****************************
     * HOLDING REGISTER MASK TESTS
     ****************************/

    /* WRITE_SINGLE_REGISTER */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nWRITE_SINGLE_REGISTER\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_write_register_macaroons(ctx, UT_REGISTERS_ADDRESS, 0x12);
#else
    rc = modbus_write_register(ctx, UT_REGISTERS_ADDRESS, 0x12);
#endif
    configASSERT(rc == 1);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* MASK_WRITE_SINGLE_REGISTER */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nMASK_WRITE_SINGLE_REGISTER\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_mask_write_register_macaroons(ctx, UT_REGISTERS_ADDRESS, 0xF2, 0x25);
#else
    rc = modbus_mask_write_register(ctx, UT_REGISTERS_ADDRESS, 0xF2, 0x25);
#endif
    configASSERT(rc != -1);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

    /* READ_SINGLE_REGISTER */
#ifndef NDEBUG
    print_shim_info("modbus_client", __FUNCTION__);
    printf("\nREAD_SINGLE_REGISTER\n");
#endif

#if defined(MACAROONS_LAYER)
    rc = modbus_read_registers_macaroons(ctx, UT_REGISTERS_ADDRESS, 1, tab_rp_registers);
#else
    rc = modbus_read_registers(ctx, UT_REGISTERS_ADDRESS, 1, tab_rp_registers);
#endif
    configASSERT(rc == 1);
    configASSERT(tab_rp_registers[0] == 0x17);
    //vTaskDelayUntil(&xNextWakeTime, modbusCLIENT_SEND_FREQUENCY_MS);

#if defined(MICROBENCHMARK)
    /* print microbenchmark samples to stdout */
    vPrintMicrobenchmarkSamples();
#endif

    printf("END_TEST\n");

    prvClientTaskShutdown();
}

/*-----------------------------------------------------------*/

/**
 * Closes the connection, frees memory, and deletes the task
 */
static void prvClientTaskShutdown(void) {
    /* close the connection to the server */
    modbus_close(ctx);

    /* free the ctx */
    modbus_free(ctx);

    /* free allocated memory */
    vPortFree(tab_rp_bits);
    vPortFree(tab_rp_registers);
    vPortFree(tab_rp_string);

#if defined(MICROBENCHMARK)
    /* kill FreeRTOS execution */
    _exit(0);
#else
    /* delete this task */
    vTaskDelete(NULL);
#endif
}

/*-----------------------------------------------------------*/

/**
 * converts a string request to a uint8_t *
 *
 * input format "[FF][FF][FF]..."
 *
 * returns request length
 * */
static int convert_string_req(const char *req_string, uint8_t *req)
{
    int rc = -1;
    char buf[3];

    if (strlen(req_string) % 4 != 0 || strlen(req_string) == 0)
    {
        return rc;
    }

    /**
     * every 4 characters in the string is one hex integer
     * i.e., "[FF]" -> 0xFF
     * */
    for (size_t i = 0; i < (strlen(req_string)) / 4; ++i)
    {
        memcpy(buf, &req_string[i * 4 + 1], 2);
        buf[2] = '\0';
        sscanf(buf, "%hhx", &req[i]);
    }

    req[(strlen(req_string)) / 4] = '\0';

    return (strlen(req_string)) / 4 + 1;
}

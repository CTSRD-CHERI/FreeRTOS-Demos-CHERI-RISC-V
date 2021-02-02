/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include "modbus/modbus.h"

#include "modbus_test_constants.h"

#if defined(MODBUS_BENCHMARK)
#include "microbenchmark.h"
#endif

#if defined(MODBUS_NETWORK_CAPS)
#include "modbus_network_caps.h"
#endif

const int EXCEPTION_RC = 2;

enum {
    TCP,
    TCP_PI,
    RTU
};

int test_body(void);
int test_server(modbus_t *ctx, int use_backend);
int send_crafted_request(modbus_t *ctx, int function,
                         uint8_t *req, int req_size,
                         uint16_t max_value, uint16_t bytes,
                         int backend_length, int backend_offset);
int equal_dword(uint16_t *tab_reg, const uint32_t value);
int convert_string_req(const char *req_string, uint8_t *req);

#define BUG_REPORT(_cond, _format, _args ...) \
    printf("\r\nLine %d: assertion error for '%s': " _format "\r\n", __LINE__, # _cond, ## _args)

#define ASSERT_TRUE(_cond, _format, __args...) {  \
    if (_cond) {                                  \
        printf("OK\r\n");                           \
    } else {                                      \
        BUG_REPORT(_cond, _format, ## __args);    \
        goto close;                               \
    }                                             \
};

int equal_dword(uint16_t *tab_reg, const uint32_t value) {
    return ((tab_reg[0] == (value >> 16)) && (tab_reg[1] == (value & 0xFFFF)));
}

static uint8_t *tab_rp_bits = NULL;
static uint8_t *tab_rp_string = NULL;
static uint16_t *tab_rp_registers = NULL;
static modbus_t *ctx = NULL;
static int nb_points;
static int nb_chars;
static uint32_t response_to_sec;
static uint32_t response_to_usec;
static int success = FALSE;

int main(int argc, char *argv[])
{
    int rc;
    int num_iters;
    struct timeval tv_start, tv_end;
    uint64_t time_diff;

    if (argc > 1) {
        if (strcmp(argv[1], "qemu") == 0) {
            ctx = modbus_new_tcp("127.0.0.1", 1502);
        } else if (strcmp(argv[1], "fett") == 0) {
            ctx = modbus_new_tcp("10.0.2.15", 502);
        } else {
            printf("Usage: %s [qemu|fett] - Modbus client for unit testing\r\n", argv[0]);
            exit(1);
        }
    } else {
        /* By default */
        ctx = modbus_new_tcp("127.0.0.1", 1502);
    }

    if (argc == 3) {
        num_iters = atoi(argv[2]);
    } else {
        num_iters = 1;
    }

    if (ctx == NULL) {
        fprintf(stderr, "Unable to allocate libmodbus context\r\n");
        return -1;
    }
    modbus_set_debug(ctx, TRUE);
    modbus_set_error_recovery(ctx,
                              MODBUS_ERROR_RECOVERY_LINK |
                              MODBUS_ERROR_RECOVERY_PROTOCOL);

    modbus_get_response_timeout(ctx, &response_to_sec, &response_to_usec);
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\r\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    /* Allocate and initialize the memory to store the bits */
    nb_points = (UT_BITS_NB > UT_INPUT_BITS_NB) ? UT_BITS_NB : UT_INPUT_BITS_NB;
    tab_rp_bits = (uint8_t *) malloc(nb_points * sizeof(uint8_t));
    memset(tab_rp_bits, 0, nb_points * sizeof(uint8_t));

    /* Allocate and initialize the memory to store the registers */
    nb_points = (UT_REGISTERS_NB > UT_INPUT_REGISTERS_NB) ?
        UT_REGISTERS_NB : UT_INPUT_REGISTERS_NB;
    tab_rp_registers = (uint16_t *) malloc(nb_points * sizeof(uint16_t));
    memset(tab_rp_registers, 0, nb_points * sizeof(uint16_t));

    /* Allocate and initialize the memory to store the string */
    tab_rp_string = (uint8_t *)malloc(MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));
    nb_chars = convert_string_req((const char *)TEST_STRING, tab_rp_string);

#if defined(MODBUS_NETWORK_CAPS)
    /* when using macaroons, the server will initialise a macaroon and store
     * it in mb_mapping->tab_string, so the client starts by reading that
     * string and initialising it's own macaroon.
     *
     * this is basically a TOFU operation: we'll deserialise it and store
     * it as the client's initial macaroon */
    printf("\r\nREAD_STRING\r\n");
    memset(tab_rp_string, 0, MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));
    rc = modbus_read_string(ctx, tab_rp_string);
    ASSERT_TRUE(rc != -1, "");

    printf("\r\nINITIALISE_CLIENT_NETWORK_CAPS\r\n");
    rc = initialise_client_network_caps(ctx, (char *)tab_rp_string, rc);
    ASSERT_TRUE(rc != -1, "");
#endif

    printf("----------\r\n");
    printf("BEGIN_TEST\r\n");
    printf("----------\r\n");


    /* Execute a series of requests to the Modbus server
     * and validate the replies. */
    for(int i = 0; i < num_iters; ++i) {
        printf("***************************\r\n");
        printf("*** CLIENT ITERATION %d ***\r\n", i);
        printf("***************************\r\n");

#if defined(MODBUS_BENCHMARK)
        /* Get timestamp before executing test_body(). */
        gettimeofday(&tv_start, NULL);
#endif

        rc = test_body();
        ASSERT_TRUE(rc != -1, "");

#if defined(MODBUS_BENCHMARK)
        /* Get timestamp after executing test_body(). */
        gettimeofday(&tv_end, NULL);

        /* Get the timestamp difference in usec and store as
         * a benchmark sample. */
        time_diff = 1e6 * (tv_end.tv_sec - tv_start.tv_sec) +
            (tv_end.tv_usec - tv_start.tv_usec);
        xMicrobenchmarkSample(MAX_PROCESSING, "MODBUS_FC_ALL", time_diff, 1);
#endif
    }

    printf("--------\r\n");
    printf("END_TEST\r\n");
    printf("--------\r\n");

#if defined(MODBUS_BENCHMARK)
    vPrintMicrobenchmarkSamples();
#endif

    success = TRUE;

close:
    /* Free the memory */
    free(tab_rp_bits);
    free(tab_rp_registers);

    /* Close the connection */
    modbus_close(ctx);
    modbus_free(ctx);
    ctx = NULL;

    return (success) ? 0 : -1;
}

/* Send crafted requests to test server resilience
   and ensure proper exceptions are returned. */
int test_server(modbus_t *ctx, int use_backend)
{
    int rc;
    int i;
    /* Read requests */
    const int READ_RAW_REQ_LEN = 6;
    const int slave = (use_backend == RTU) ? SERVER_ID : MODBUS_TCP_SLAVE;
    uint8_t read_raw_req[] = {
        slave,
        /* function, address, 5 values */
        MODBUS_FC_READ_HOLDING_REGISTERS,
        UT_REGISTERS_ADDRESS >> 8, UT_REGISTERS_ADDRESS & 0xFF,
        0x0, 0x05
    };
    /* Write and read registers request */
    const int RW_RAW_REQ_LEN = 13;
    uint8_t rw_raw_req[] = {
        slave,
        /* function, addr to read, nb to read */
        MODBUS_FC_WRITE_AND_READ_REGISTERS,
        /* Read */
        UT_REGISTERS_ADDRESS >> 8, UT_REGISTERS_ADDRESS & 0xFF,
        (MODBUS_MAX_WR_READ_REGISTERS + 1) >> 8,
        (MODBUS_MAX_WR_READ_REGISTERS + 1) & 0xFF,
        /* Write */
        0, 0,
        0, 1,
        /* Write byte count */
        1 * 2,
        /* One data to write... */
        0x12, 0x34
    };
    const int WRITE_RAW_REQ_LEN = 13;
    uint8_t write_raw_req[] = {
        slave,
        /* function will be set in the loop */
        MODBUS_FC_WRITE_MULTIPLE_REGISTERS,
        /* Address */
        UT_REGISTERS_ADDRESS >> 8, UT_REGISTERS_ADDRESS & 0xFF,
        /* 3 values, 6 bytes */
        0x00, 0x03, 0x06,
        /* Dummy data to write */
        0x02, 0x2B, 0x00, 0x01, 0x00, 0x64
    };
    const int INVALID_FC = 0x42;
    const int INVALID_FC_REQ_LEN = 6;
    uint8_t invalid_fc_raw_req[] = {
        slave, 0x42, 0x00, 0x00, 0x00, 0x00
    };

    int req_length;
    uint8_t rsp[MODBUS_TCP_MAX_ADU_LENGTH];
    int tab_read_function[] = {
        MODBUS_FC_READ_COILS,
        MODBUS_FC_READ_DISCRETE_INPUTS,
        MODBUS_FC_READ_HOLDING_REGISTERS,
        MODBUS_FC_READ_INPUT_REGISTERS
    };
    int tab_read_nb_max[] = {
        MODBUS_MAX_READ_BITS + 1,
        MODBUS_MAX_READ_BITS + 1,
        MODBUS_MAX_READ_REGISTERS + 1,
        MODBUS_MAX_READ_REGISTERS + 1
    };
    int backend_length;
    int backend_offset;

    if (use_backend == RTU) {
        backend_length = 3;
        backend_offset = 1;
    } else {
        backend_length = 7;
        backend_offset = 7;
    }

    printf("\r\nTEST RAW REQUESTS:\r\n");

    uint32_t old_response_to_sec;
    uint32_t old_response_to_usec;

    /* This requests can generate flushes server side so we need a higher
     * response timeout than the server. The server uses the defined response
     * timeout to sleep before flushing.
     * The old timeouts are restored at the end.
     */
    modbus_get_response_timeout(ctx, &old_response_to_sec, &old_response_to_usec);
    modbus_set_response_timeout(ctx, 0, 600000);

    req_length = modbus_send_raw_request(ctx, read_raw_req, READ_RAW_REQ_LEN);
    printf("* modbus_send_raw_request: ");
    ASSERT_TRUE(req_length == (backend_length + 5), "FAILED (%d)\r\n", req_length);

    printf("* modbus_receive_confirmation: ");
    rc = modbus_receive_confirmation(ctx, rsp);
    ASSERT_TRUE(rc == (backend_length + 12), "FAILED (%d)\r\n", rc);

    /* Try to read more values than a response could hold for all data
       types. */
    for (i=0; i<4; i++) {
        rc = send_crafted_request(ctx, tab_read_function[i],
                                  read_raw_req, READ_RAW_REQ_LEN,
                                  tab_read_nb_max[i], 0,
                                  backend_length, backend_offset);
        if (rc == -1)
            goto close;
    }

    rc = send_crafted_request(ctx, MODBUS_FC_WRITE_AND_READ_REGISTERS,
                              rw_raw_req, RW_RAW_REQ_LEN,
                              MODBUS_MAX_WR_READ_REGISTERS + 1, 0,
                              backend_length, backend_offset);
    if (rc == -1)
        goto close;

    rc = send_crafted_request(ctx, MODBUS_FC_WRITE_MULTIPLE_REGISTERS,
                              write_raw_req, WRITE_RAW_REQ_LEN,
                              MODBUS_MAX_WRITE_REGISTERS + 1, 6,
                              backend_length, backend_offset);
    if (rc == -1)
        goto close;

    rc = send_crafted_request(ctx, MODBUS_FC_WRITE_MULTIPLE_COILS,
                              write_raw_req, WRITE_RAW_REQ_LEN,
                              MODBUS_MAX_WRITE_BITS + 1, 6,
                              backend_length, backend_offset);
    if (rc == -1)
        goto close;

    /* Modbus write multiple registers with large number of values but a set a
       small number of bytes in requests (not nb * 2 as usual). */
    rc = send_crafted_request(ctx, MODBUS_FC_WRITE_MULTIPLE_REGISTERS,
                              write_raw_req, WRITE_RAW_REQ_LEN,
                              MODBUS_MAX_WRITE_REGISTERS, 6,
                              backend_length, backend_offset);
    if (rc == -1)
        goto close;

    rc = send_crafted_request(ctx, MODBUS_FC_WRITE_MULTIPLE_COILS,
                              write_raw_req, WRITE_RAW_REQ_LEN,
                              MODBUS_MAX_WRITE_BITS, 6,
                              backend_length, backend_offset);
    if (rc == -1)
        goto close;

    /* Test invalid function code */
    modbus_send_raw_request(ctx, invalid_fc_raw_req, INVALID_FC_REQ_LEN * sizeof(uint8_t));
    rc = modbus_receive_confirmation(ctx, rsp);
    printf("Return an exception on unknown function code: ");
    ASSERT_TRUE(rc == (backend_length + EXCEPTION_RC) &&
                rsp[backend_offset] == (0x80 + INVALID_FC), "")

    modbus_set_response_timeout(ctx, old_response_to_sec, old_response_to_usec);
    return 0;
close:
    modbus_set_response_timeout(ctx, old_response_to_sec, old_response_to_usec);
    return -1;
}


int send_crafted_request(modbus_t *ctx, int function,
                         uint8_t *req, int req_len,
                         uint16_t max_value, uint16_t bytes,
                         int backend_length, int backend_offset)
{
    uint8_t rsp[MODBUS_TCP_MAX_ADU_LENGTH];
    int j;

    for (j=0; j<2; j++) {
        int rc;

        req[1] = function;
        if (j == 0) {
            /* Try to read or write zero values on first iteration */
            req[4] = 0x00;
            req[5] = 0x00;
            if (bytes) {
                /* Write query */
                req[6] = 0x00;
            }
        } else {
            /* Try to read or write max values + 1 on second iteration */
            req[4] = (max_value >> 8) & 0xFF;
            req[5] = max_value & 0xFF;
            if (bytes) {
                /* Write query (nb values * 2 to convert in bytes for registers) */
                req[6] = bytes;
            }
        }

        modbus_send_raw_request(ctx, req, req_len * sizeof(uint8_t));
        if (j == 0) {
            printf("* try function 0x%X: %s 0 values: ", function, bytes ? "write": "read");
        } else {
            printf("* try function 0x%X: %s %d values: ", function, bytes ? "write": "read",
                   max_value);
        }
        rc = modbus_receive_confirmation(ctx, rsp);
        ASSERT_TRUE(rc == (backend_length + EXCEPTION_RC) &&
                    rsp[backend_offset] == (0x80 + function) &&
                    rsp[backend_offset + 1] == MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE, "");
    }
    return 0;
close:
    return -1;
}

/**
 * converts a string request to a uint8_t *
 *
 * input format "[FF][FF][FF]..."
 *
 * returns request length
 * */
int convert_string_req(const char *req_string, uint8_t *req)
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

/**
 * Execute a series of requests to the Modbus server
 * and validate the replies.
 **/
int test_body(void)
{
    int rc;

    /**************
     * STRING TESTS
     **************/

    /* WRITE_STRING */
    printf("\r\nWRITE_STRING\r\n");
    rc = modbus_write_string(ctx, tab_rp_string, nb_chars);
    ASSERT_TRUE(rc != -1, "");

    /* READ_STRING */
    printf("\r\nREAD_STRING\r\n");
    memset(tab_rp_string, 0, MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));
    rc = modbus_read_string(ctx, tab_rp_string);
    ASSERT_TRUE(rc != -1, "");

    /************
     * COIL TESTS
     ***********/

    /* WRITE_SINGLE_COIL */
    printf("\r\nWRITE_SINGLE_COIL\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_write_bit_network_caps(ctx, UT_BITS_ADDRESS, ON);
#else
    rc = modbus_write_bit(ctx, UT_BITS_ADDRESS, ON);
#endif
    ASSERT_TRUE(rc == 1, "");

    /* READ_SINGLE_COIL */
    printf("\r\nREAD_SINGLE_COIL\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_read_bits_network_caps(ctx, UT_BITS_ADDRESS, 1, tab_rp_bits);
#else
    rc = modbus_read_bits(ctx, UT_BITS_ADDRESS, 1, tab_rp_bits);
#endif
	ASSERT_TRUE(rc == 1, "FAILED (nb points %d)\r\n", rc);
    ASSERT_TRUE(tab_rp_bits[0] == ON, "FAILED (%0X != %0X)\r\n",
                tab_rp_bits[0], ON);

    /* WRITE_MULTIPLE_COILS */
    printf("\r\nWRITE_MULTIPLE_COILS\r\n");
    {
        uint8_t tab_value[UT_BITS_NB];
        modbus_set_bits_from_bytes(tab_value, 0, UT_BITS_NB, UT_BITS_TAB);

#if defined(MODBUS_NETWORK_CAPS)
        rc = modbus_write_bits_network_caps(ctx, UT_BITS_ADDRESS, UT_BITS_NB, tab_value);
#else
        rc = modbus_write_bits(ctx, UT_BITS_ADDRESS, UT_BITS_NB, tab_value);
#endif
    }
    ASSERT_TRUE(rc == UT_BITS_NB, "");

    /* READ_MULTIPLE_COILS */
    printf("\r\nREAD_MULTIPLE_COILS\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_read_bits_network_caps(ctx, UT_BITS_ADDRESS, UT_BITS_NB, tab_rp_bits);
#else
    rc = modbus_read_bits(ctx, UT_BITS_ADDRESS, UT_BITS_NB, tab_rp_bits);
#endif
	ASSERT_TRUE(rc == UT_BITS_NB, "FAILED (nb points %d)\r\n", rc);

	{
		int i = 0;
        uint8_t value;
		nb_points = UT_BITS_NB;
		while (nb_points > 0) {
			int nb_bits = (nb_points > 8) ? 8 : nb_points;

			value = modbus_get_byte_from_bits(tab_rp_bits, i*8, nb_bits);
			ASSERT_TRUE(value == UT_BITS_TAB[i], "FAILED (%0X != %0X)\r\n",
					value, UT_BITS_TAB[i]);

			nb_points -= nb_bits;
			i++;
		}
	}

    /**********************
     * DISCRETE INPUT TESTS
     *********************/

    /* READ_INPUT_BITS */
    printf("\r\nREAD_INPUT_BITS\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_read_input_bits_network_caps(ctx, UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB, tab_rp_bits);
#else
    rc = modbus_read_input_bits(ctx, UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB, tab_rp_bits);
#endif
    ASSERT_TRUE(rc == UT_INPUT_BITS_NB, "FAILED (nb points %d)\r\n", rc);

    {
        /* further checks on the returned discrete inputs */
        int i = 0;
        uint8_t value;
        nb_points = UT_INPUT_BITS_NB;
        while (nb_points > 0) {
            int nb_bits = (nb_points > 8) ? 8 : nb_points;
            value = modbus_get_byte_from_bits(tab_rp_bits, i*8, nb_bits);
			ASSERT_TRUE(value == UT_INPUT_BITS_TAB[i], "FAILED (%0X != %0X)\r\n",
					value, UT_INPUT_BITS_TAB[i]);

            nb_points -= nb_bits;
            i++;
        }
    }

    /************************
     * HOLDING REGISTER TESTS
     ***********************/

    /* WRITE_SINGLE_REGISTER */
    printf("\r\nWRITE_SINGLE_REGISTER\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_write_register_network_caps(ctx, UT_REGISTERS_ADDRESS, 0x1234);
#else
    rc = modbus_write_register(ctx, UT_REGISTERS_ADDRESS, 0x1234);
#endif
    ASSERT_TRUE(rc == 1, "");

    /* READ_SINGLE_REGISTER */
    printf("\r\nREAD_SINGLE_REGISTER\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_read_registers_network_caps(ctx, UT_REGISTERS_ADDRESS, 1, tab_rp_registers);
#else
    rc = modbus_read_registers(ctx, UT_REGISTERS_ADDRESS, 1, tab_rp_registers);
#endif
    ASSERT_TRUE(rc == 1, "FAILED (nb points %d)\r\n", rc);
    ASSERT_TRUE(tab_rp_registers[0] == 0x1234, "FAILED (%0X != %0X)\r\n",
                tab_rp_registers[0], 0x1234);

    /* WRITE_MULTIPLE_REGISTERS */
    printf("\r\nWRITE_MULTIPLE_REGISTERS\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_write_registers_network_caps(ctx, UT_REGISTERS_ADDRESS, UT_REGISTERS_NB, UT_REGISTERS_TAB);
#else
    rc = modbus_write_registers(ctx, UT_REGISTERS_ADDRESS, UT_REGISTERS_NB, UT_REGISTERS_TAB);
#endif
    ASSERT_TRUE(rc == UT_REGISTERS_NB, "");

    /* READ_MULTIPLE_REGISTERS */
    printf("\r\nREAD_MULTIPLE_REGISTERS\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_read_registers_network_caps(ctx, UT_REGISTERS_ADDRESS, UT_REGISTERS_NB, tab_rp_registers);
#else
    rc = modbus_read_registers(ctx, UT_REGISTERS_ADDRESS, UT_REGISTERS_NB, tab_rp_registers);
#endif
    ASSERT_TRUE(rc == UT_REGISTERS_NB, "FAILED (nb points %d)\r\n", rc);

    for (int i=0; i < UT_REGISTERS_NB; i++) {
        ASSERT_TRUE(tab_rp_registers[i] == UT_REGISTERS_TAB[i],
                    "FAILED (%0X != %0X)\r\n",
                    tab_rp_registers[i], UT_REGISTERS_TAB[i]);
    }

	/* Attempt to read 0 registers.  Expected to fail.
     * We don't include expected failures in microbenchmarking,
     * since the generally result in timeouts, which is both slow and
     * not really helpful in any way. */
#if !defined(MODBUS_BENCHMARK)
#if defined(MODBUS_NETWORK_CAPS)
	rc = modbus_read_registers_network_caps(ctx, UT_REGISTERS_ADDRESS,
			0, tab_rp_registers);
#else
	rc = modbus_read_registers(ctx, UT_REGISTERS_ADDRESS,
			0, tab_rp_registers);
#endif /* defined(MODBUS_NETWORK_CAPS) */
    ASSERT_TRUE(rc == -1, "FAILED (nb_points %d)\r\n", rc);
#endif /* !defined(MODBUS_BENCHMARK) */

    /* WRITE_AND_READ_REGISTERS */
    printf("\r\nWRITE_AND_READ_REGISTERS\r\n");

    nb_points = (UT_REGISTERS_NB > UT_INPUT_REGISTERS_NB) ? UT_REGISTERS_NB : UT_INPUT_REGISTERS_NB;
    memset(tab_rp_registers, 0, nb_points * sizeof(uint16_t));

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_write_and_read_registers_network_caps(ctx,
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
    ASSERT_TRUE(rc == UT_REGISTERS_NB, "FAILED (nb points %d != %d)\r\n",
                rc, UT_REGISTERS_NB);

    ASSERT_TRUE(tab_rp_registers[0] == UT_REGISTERS_TAB[0],
                "FAILED (%0X != %0X)\r\n",
                tab_rp_registers[0], UT_REGISTERS_TAB[0]);

    for (int i=1; i < UT_REGISTERS_NB; i++) {
        ASSERT_TRUE(tab_rp_registers[i] == 0, "FAILED (%0X != %0X)\r\n",
                    tab_rp_registers[i], 0);
    }

    /**********************
     * INPUT REGISTER TESTS
     *********************/

    /* READ_INPUT_REGISTERS */
    printf("\r\nREAD_INPUT_REGISTERS\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_read_input_registers_network_caps(ctx, UT_INPUT_REGISTERS_ADDRESS,
            UT_INPUT_REGISTERS_NB, tab_rp_registers);
#else
    rc = modbus_read_input_registers(ctx, UT_INPUT_REGISTERS_ADDRESS,
            UT_INPUT_REGISTERS_NB, tab_rp_registers);
#endif
    ASSERT_TRUE(rc == UT_INPUT_REGISTERS_NB, "FAILED (nb points %d)\r\n", rc);

    for (int i=0; i < UT_INPUT_REGISTERS_NB; i++) {
        ASSERT_TRUE(tab_rp_registers[i] == UT_INPUT_REGISTERS_TAB[i],
                    "FAILED (%0X != %0X)\r\n",
                    tab_rp_registers[i], UT_INPUT_REGISTERS_TAB[i]);
    }

    /*****************************
     * HOLDING REGISTER MASK TESTS
     ****************************/

    /* WRITE_SINGLE_REGISTER */
    printf("\r\nWRITE_SINGLE_REGISTER\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_write_register_network_caps(ctx, UT_REGISTERS_ADDRESS, 0x12);
#else
    rc = modbus_write_register(ctx, UT_REGISTERS_ADDRESS, 0x12);
#endif
    ASSERT_TRUE(rc == 1, "");

    /* MASK_WRITE_SINGLE_REGISTER */
    printf("\r\nMASK_WRITE_SINGLE_REGISTER\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_mask_write_register_network_caps(ctx, UT_REGISTERS_ADDRESS, 0xF2, 0x25);
#else
    rc = modbus_mask_write_register(ctx, UT_REGISTERS_ADDRESS, 0xF2, 0x25);
#endif
    ASSERT_TRUE(rc != -1, "FAILED (%x == -1)\r\n", rc);

    /* READ_SINGLE_REGISTER */
    printf("\r\nREAD_SINGLE_REGISTER\r\n");

#if defined(MODBUS_NETWORK_CAPS)
    rc = modbus_read_registers_network_caps(ctx, UT_REGISTERS_ADDRESS, 1, tab_rp_registers);
#else
    rc = modbus_read_registers(ctx, UT_REGISTERS_ADDRESS, 1, tab_rp_registers);
#endif
    ASSERT_TRUE(rc == 1, "");
    ASSERT_TRUE(tab_rp_registers[0] == 0x17,
                "FAILED (%0X != %0X)\r\n",
                tab_rp_registers[0], 0x17);

    return 0;

close:
    return -1;
}

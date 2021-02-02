/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Michael Dodson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/

#include "modbus_network_caps.h"

/**
 * Variables to hold Macaroon properties for either
 * the Modbus client or Modbus server
 */
static unsigned char *key_;
static size_t key_sz_;

static unsigned char *id_;
static size_t id_sz_;

static unsigned char *location_;
static size_t location_sz_;

static struct macaroon *client_macaroon_;
static struct macaroon *server_macaroon_;

/***********
 * CONSTANTS
 **********/
#define MAX_MACAROON_INITIALISATION_LENGTH 256
#define MAX_CAVEATS 10
#define MAX_CAVEAT_LENGTH 40
#define FUNCTION_CAVEAT_TOKEN "function = "
#define ADDRESS_CAVEAT_TOKEN "address = "

/******************
 * HELPER FUNCTIONS
 *****************/

/*
 * Takes a bitfield representing a composite of one or more function codes and
 * creates a string in the form "function = [integer]" where [integer] is the
 * interpretation of the bitfield.
 *
 * e.g., the composition of MODBUS_FC_READ_COILS (0x01) and MODBUS_FC_READ_DISCRETE_INPUTS (0x02)
 * results in the bitfield 110, so the corresponding string would be "function = 6"
 */
static unsigned char *create_function_caveat_from_bitfield(int function_code_bitfield)
{
    char function_code_bitfield_string[MAX_CAVEAT_LENGTH];
#if defined(__freertos__)
    char *function_caveat = (char *)pvPortMalloc(MAX_CAVEAT_LENGTH * sizeof(char));
#else
    char *function_caveat = (char *)malloc(MAX_CAVEAT_LENGTH * sizeof(char));
#endif

    strncpy(function_caveat, FUNCTION_CAVEAT_TOKEN, MAX_CAVEAT_LENGTH);
    snprintf(function_code_bitfield_string, MAX_CAVEAT_LENGTH, "%d", function_code_bitfield);
    strncat(function_caveat, function_code_bitfield_string,
            MAX_CAVEAT_LENGTH - strnlen(FUNCTION_CAVEAT_TOKEN, MAX_CAVEAT_LENGTH));

    return (unsigned char *)function_caveat;
}

/*
 * Takes a composite (READ-ONLY or WRITE-ONLY) and constructs a bitfield to pass to
 * create_function_caveat_from_bitfield()
 */
static unsigned char *create_function_caveat_from_composite(const char *function_code_composite)
{
    uint32_t function_code_bitfield = 0;

    if (strcmp(function_code_composite, "READ-ONLY") == 0)
    {
        function_code_bitfield |= 1 << MODBUS_FC_READ_COILS;
        function_code_bitfield |= 1 << MODBUS_FC_READ_DISCRETE_INPUTS;
        function_code_bitfield |= 1 << MODBUS_FC_READ_HOLDING_REGISTERS;
        function_code_bitfield |= 1 << MODBUS_FC_READ_INPUT_REGISTERS;
        function_code_bitfield |= 1 << MODBUS_FC_READ_EXCEPTION_STATUS;
        function_code_bitfield |= 1 << MODBUS_FC_REPORT_SLAVE_ID;
        function_code_bitfield |= 1 << MODBUS_FC_READ_STRING;
    }
    else if (strcmp(function_code_composite, "WRITE-ONLY") == 0)
    {
        function_code_bitfield |= 1 << MODBUS_FC_WRITE_SINGLE_COIL;
        function_code_bitfield |= 1 << MODBUS_FC_WRITE_SINGLE_REGISTER;
        function_code_bitfield |= 1 << MODBUS_FC_WRITE_MULTIPLE_COILS;
        function_code_bitfield |= 1 << MODBUS_FC_WRITE_MULTIPLE_REGISTERS;
        function_code_bitfield |= 1 << MODBUS_FC_MASK_WRITE_REGISTER;
        function_code_bitfield |= 1 << MODBUS_FC_WRITE_STRING;
    }
    else
    {
        return (unsigned char *)"\0";
    }

    return create_function_caveat_from_bitfield(function_code_bitfield);
}

/*
 * Takes an array of function codes (e.g., int fc[] = {MODBUS_FC_READ_COILS, MODBUS_FC_READ_DISCRETE_INPUTS})
 * creates a bitfield and calls create_function_caveat_from_bitfield()
 */
static unsigned char *create_function_caveat_from_fc_array(int *function_codes, int num_codes)
{
    uint32_t function_code_bitfield = 0;
    for (int i = 0; i < num_codes; ++i)
    {
        function_code_bitfield |= 1 << function_codes[i];
    }
    return create_function_caveat_from_bitfield(function_code_bitfield);
}

/*
 * Takes a single function code (e.g., MODBUS_FC_READ_COILS), creates a bitfield and
 * calls create_function_caveat_from_bitfield()
 */
static unsigned char *create_function_caveat_from_fc(int function_code)
{
    return create_function_caveat_from_bitfield(1 << function_code);
}

// Below function extracts characters present in src
// between m and n (excluding n)
static char *substr(const char *src, int m, int n)
{
    // get length of the destination string
    int len = n - m;

    // allocate (len + 1) chars for destination (+1 for extra null character)
#if defined(__freertos__)
    char *dest = (char *)pvPortMalloc(sizeof(char) * (len + 1));
#else
    char *dest = (char *)malloc(sizeof(char) * (len + 1));
#endif

    // start with m'th char and copy 'len' chars into destination
    strncpy(dest, (src + m), len);
    dest[len] = '\0';

    // return the destination string
    return dest;
}

/**
 * Verifies that the function caveats are not mutually exclusive
 * e.g., that we don't have both READ-ONLY and WRITE-ONLY
 * */
static int check_function_caveats(unsigned char *first_party_caveats[], int num_caveats)
{
    int token_length = strnlen(FUNCTION_CAVEAT_TOKEN, MAX_CAVEAT_LENGTH);
    unsigned char *tmp;
    int tmp_length;
    char *fc_str;

    uint32_t fc = 0xFFFFFFFF;
    for (int i = 0; i < num_caveats; ++i)
    {
        tmp = first_party_caveats[i];
        tmp_length = strnlen((char *)tmp, MAX_CAVEAT_LENGTH);

        if (strncmp((char *)tmp, FUNCTION_CAVEAT_TOKEN, token_length) == 0)
        {
            fc_str = substr((char *)tmp, token_length, tmp_length);
            fc &= atoi(fc_str);
#if defined(__freertos__)
            vPortFree(fc_str);
#else
            free(fc_str);
#endif
        }
    }

    if (fc > 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * Create an address caveat
 *
 * address = 0xABCDEFGH
 * ABCD is the min address
 * EFGH is the max address
 * */
static unsigned char *create_address_caveat(uint16_t min_address, uint16_t max_address)
{
    uint32_t address_composed = (min_address << 16) + max_address;
    char address_composed_string[MAX_CAVEAT_LENGTH];
#if defined(__freertos__)
    char *address_caveat = (char *)pvPortMalloc(MAX_CAVEAT_LENGTH * sizeof(char));
#else
    char *address_caveat = (char *)malloc(MAX_CAVEAT_LENGTH * sizeof(char));
#endif

    strncpy(address_caveat, ADDRESS_CAVEAT_TOKEN, MAX_CAVEAT_LENGTH);
    snprintf(address_composed_string, MAX_CAVEAT_LENGTH, "%d", address_composed);
    strncat(address_caveat, address_composed_string,
            MAX_CAVEAT_LENGTH - strnlen(ADDRESS_CAVEAT_TOKEN, MAX_CAVEAT_LENGTH));

    return (unsigned char *)address_caveat;
}

/**
 * Verifies that the addresses in the request are not excluded by
 * address caveats
 * */
static int check_address_caveats(unsigned char *first_party_caveats[], int num_caveats, unsigned char *address_request)
{
    unsigned char *fpc;
    int fpc_length;
    char *ar_str;

    /* convert address request to an int */
    int token_length = strnlen(ADDRESS_CAVEAT_TOKEN, MAX_CAVEAT_LENGTH);
    ar_str = substr((char *)address_request, token_length, strnlen((char *)address_request, MAX_CAVEAT_LENGTH));
    uint32_t ar = atoi(ar_str);
#if defined(__freertos__)
    vPortFree(ar_str);
#else
    free(ar_str);
#endif

    uint16_t ar_min = (0xFFFF0000 & ar) >> 16;
    uint16_t ar_max = 0x0000FFFF & ar;

    uint32_t ac;
    uint16_t ac_min;
    uint16_t ac_max;

    /**
     * iterate through all address caveats, extract min and max
     *
     * if the requested address min and max are outside the bounds
     * of any caveat, return -1.  otherwise return 0.
     * */
    for (int i = 0; i < num_caveats; ++i)
    {
        fpc = first_party_caveats[i];
        fpc_length = strnlen((char *)fpc, MAX_CAVEAT_LENGTH);

        if (strncmp((char *)fpc, ADDRESS_CAVEAT_TOKEN, token_length) == 0)
        {
            ac = atoi(substr((char *)fpc, token_length, strnlen((char *)fpc, MAX_CAVEAT_LENGTH)));
            ac_min = (0xFFFF0000 & ac) >> 16;
            ac_max = 0x0000FFFF & ac;

            if (ar_min < ac_min || ar_max > ac_max)
            {
                return -1;
            }
        }
    }

    return 0;
}

/**
 * Based on function, address, and number, calculate
 * the maximum address expected to be accessed.
 *
 * For bitwise operations (e.g., read_bits), round up
 * to the nearest byte
 * */
static uint16_t find_max_address(int function, uint16_t addr, int nb)
{
    uint16_t addr_max;

    switch (function)
    {
        case MODBUS_FC_READ_COILS:
        case MODBUS_FC_WRITE_MULTIPLE_COILS:
        case MODBUS_FC_READ_DISCRETE_INPUTS:
            addr_max = addr + (nb / 8) + ((nb % 8) ? 1 : 0);
            break;
        case MODBUS_FC_READ_HOLDING_REGISTERS:
        case MODBUS_FC_READ_INPUT_REGISTERS:
        case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
        case MODBUS_FC_WRITE_AND_READ_REGISTERS:
            addr_max = addr + (nb * 2);
            break;
        case MODBUS_FC_WRITE_SINGLE_COIL:
        case MODBUS_FC_REPORT_SLAVE_ID:
        case MODBUS_FC_READ_EXCEPTION_STATUS:
            addr_max = addr;
            break;
        case MODBUS_FC_WRITE_SINGLE_REGISTER:
        case MODBUS_FC_MASK_WRITE_REGISTER:
            addr_max = addr + 2;
            break;
        case MODBUS_FC_WRITE_STRING:
            addr_max = addr + nb;
            break;
        default:
            addr_max = addr;
    }

    return addr_max;
}

/******************
 * CLIENT FUNCTIONS
 *****************/
int initialise_client_network_caps(modbus_t *ctx, char *serialised_macaroon, int serialised_macaroon_length)
{
    enum macaroon_returncode err = MACAROON_SUCCESS;

    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    /**
     * Deserialise the string into a Macaroon
     * */
    client_macaroon_ = macaroon_deserialize(serialised_macaroon,
            serialised_macaroon_length, &err);
    if (err != MACAROON_SUCCESS)
    {
        if (modbus_get_debug(ctx))
        {
            printf("Failed to deserialise the server macaroon from the queue\n");
            printf("err: %d\n", err);
        }
        return -1;
    }

    return 0;
}

static int send_network_caps(modbus_t *ctx, int function, uint16_t addr, int nb)
{
    int rc;
    struct macaroon *temp_macaroon;
    enum macaroon_returncode err = MACAROON_SUCCESS;

    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (client_macaroon_ == NULL)
    {
        if (modbus_get_debug(ctx))
        {
            printf("> Macaroon not initialised\n");
        }
        return -1;
    }

    /* add the function as a caveat to a temporary Macaroon*/
    unsigned char *function_caveat = create_function_caveat_from_fc(function);
    temp_macaroon = macaroon_add_first_party_caveat(
            client_macaroon_,
            function_caveat,
            strnlen((char *)function_caveat, MAX_CAVEAT_LENGTH),
            &err);
#if defined(__freertos__)
    vPortFree(function_caveat);
#else
    free(function_caveat);
#endif

    if (err != MACAROON_SUCCESS)
    {
        return -1;
    }

    /* add the address range as a caveat to a temporary Macaroon*/
    uint16_t addr_max = find_max_address(function, addr, nb);
    unsigned char *address_caveat = create_address_caveat(addr, addr_max);
    temp_macaroon = macaroon_add_first_party_caveat(
            temp_macaroon,
            address_caveat,
            strnlen((char *)address_caveat, MAX_CAVEAT_LENGTH),
            &err);
    if (err != MACAROON_SUCCESS)
    {
        return -1;
    }

    /* serialise the Macaroon and send it to the server */
    if (modbus_get_debug(ctx))
    {
        /* inspect the Macaroon */
        int buf_sz = macaroon_inspect_size_hint(temp_macaroon);
#if defined(__freertos__)
        char *buf = (char *)pvPortMalloc(buf_sz * sizeof(unsigned char));
#else
        char *buf = (char *)malloc(buf_sz * sizeof(unsigned char));
#endif
        macaroon_inspect(temp_macaroon, buf, buf_sz, &err);
        if (err != MACAROON_SUCCESS)
        {
            return -1;
        }

        printf("> sending Macaroon\n");
        printf("%s\n", buf);
        printf("%s\n", DISPLAY_MARKER);

#if defined(__freertos__)
        vPortFree(buf);
#else
        free(buf);
#endif
    }

    int msg_length = macaroon_serialize_size_hint(temp_macaroon, MACAROON_V1);
#if defined(__freertos__)
    unsigned char *msg = (unsigned char *)pvPortMalloc(msg_length * sizeof(unsigned char));
#else
    unsigned char *msg = (unsigned char *)malloc(msg_length * sizeof(unsigned char));
#endif

    macaroon_serialize(temp_macaroon, MACAROON_V1, msg, msg_length, &err);
    if (err != MACAROON_SUCCESS)
    {
        return -1;
    }

    rc = modbus_write_string(ctx, msg, msg_length);

    macaroon_destroy(temp_macaroon);

    if (rc == msg_length)
    {
        if (modbus_get_debug(ctx))
        {
            print_shim_info("macaroons_shim", __FUNCTION__);
            printf("> Macaroon response received\n");
            printf("%s\n", DISPLAY_MARKER);
        }
        return 0;
    }
    else
    {
        if (modbus_get_debug(ctx))
        {
            print_shim_info("macaroons_shim", __FUNCTION__);
            printf("> Macaroon response failed\n");
            printf("%s\n", DISPLAY_MARKER);
        }
        return -1;
    }
}

/**
 * Shim for modbus_read_bits()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_READ_COILS command
 * 2. Reads the boolean status of bits and sets the array elements
 *    in the destination to TRUE or FALSE (single bits)
 * */
int modbus_read_bits_network_caps(modbus_t *ctx, int addr, int nb, uint8_t *dest)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_READ_COILS, addr, nb) == 0)
    {
        return modbus_read_bits(ctx, addr, nb, dest);
    }

    return -1;
}

/**
 * Shim for modbus_read_input_bits()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_READ_DISCRETE_INPUTS command
 * 2. Same as modbus_read_bits but reads the remote device input table
 * */
int modbus_read_input_bits_network_caps(modbus_t *ctx, int addr, int nb, uint8_t *dest)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_READ_DISCRETE_INPUTS, addr, nb) == 0)
    {
        return modbus_read_input_bits(ctx, addr, nb, dest);
    }

    return -1;
}

/**
 * Shim for modbus_read_registers()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_READ_HOLDING_REGISTERS command
 * 2. Reads the holding registers of remote device and put the data into an
 *    array
 * */
int modbus_read_registers_network_caps(modbus_t *ctx, int addr, int nb, uint16_t *dest)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_READ_HOLDING_REGISTERS, addr, nb) == 0)
    {
        return modbus_read_registers(ctx, addr, nb, dest);
    }

    return -1;
}

/**
 * Shim for modbus_read_input_registers()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_READ_INPUT_REGISTERS command
 * 2. Reads the holding registers of remote device and put the data into an
 *    array
 * */
int modbus_read_input_registers_network_caps(modbus_t *ctx, int addr, int nb, uint16_t *dest)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_READ_INPUT_REGISTERS, addr, nb) == 0)
    {
        return modbus_read_input_registers(ctx, addr, nb, dest);
    }

    return -1;
}

/**
 * Shim for modbus_write_bit()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_WRITE_SINGLE_COIL command
 * 2. Turns ON or OFF a single bit of the remote device
 * */
int modbus_write_bit_network_caps(modbus_t *ctx, int addr, int status)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_WRITE_SINGLE_COIL, addr, 0) == 0)
    {
        return modbus_write_bit(ctx, addr, status);
    }

    return -1;
}

/**
 * Shim for modbus_write_register()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_WRITE_SINGLE_REGISTER command
 * 2. Writes a value in one register of the remote device
 * */
int modbus_write_register_network_caps(modbus_t *ctx, int addr, const uint16_t value)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_WRITE_SINGLE_REGISTER, addr, 0) == 0)
    {
        return modbus_write_register(ctx, addr, value);
    }

    return -1;
}

/**
 * Shim for modbus_write_bits()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_WRITE_SINGLE_COIL command
 * 2. Write the bits of the array in the remote device
 * */
int modbus_write_bits_network_caps(modbus_t *ctx, int addr, int nb, const uint8_t *src)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_WRITE_MULTIPLE_COILS, addr, nb) == 0)
    {
        return modbus_write_bits(ctx, addr, nb, src);
    }

    return -1;
}

/**
 * Shim for modbus_write_registers()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_WRITE_MULTIPLE_REGISTERS command
 * 2. Write the values from the array to the registers of the remote device
 * */
int modbus_write_registers_network_caps(modbus_t *ctx, int addr, int nb, const uint16_t *data)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_WRITE_MULTIPLE_REGISTERS, addr, nb) == 0)
    {
        return modbus_write_registers(ctx, addr, nb, data);
    }

    return -1;
}

/**
 * Shim for modbus_mask_write_register()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_MASK_WRITE_REGISTER command
 * 2. I'm not actually sure what this does...
 *    The unit test appears designed to fail
 * */
int modbus_mask_write_register_network_caps(modbus_t *ctx, int addr,
        uint16_t and_mask, uint16_t or_mask)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_MASK_WRITE_REGISTER, addr, 0) == 0)
    {
        return modbus_mask_write_register(ctx, addr, and_mask, or_mask);
    }

    return -1;
}

/**
 * Shim for modbus_write_and_read_registers()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_WRITE_AND_READ_REGISTERS command
 * 2. Write multiple registers from src array to remote device and read multiple
 *    registers from remote device to dest array
 * */
int modbus_write_and_read_registers_network_caps(modbus_t *ctx, int write_addr,
        int write_nb, const uint16_t *src,
        int read_addr, int read_nb,
        uint16_t *dest)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    /**
     * send_network_caps() will create an address range caveat
     * we need to find the entire range that this function is trying to access
     * since there's no way to have disjoint caveats
     * */
    uint16_t write_addr_max = find_max_address(MODBUS_FC_WRITE_AND_READ_REGISTERS, write_addr, write_nb);
    uint16_t read_addr_max = find_max_address(MODBUS_FC_WRITE_AND_READ_REGISTERS, read_addr, read_nb);
    uint16_t addr = (write_addr < read_addr) ? write_addr : read_addr;
    int nb = ((write_addr < read_addr) ? (read_addr_max - write_addr) : (write_addr_max - read_addr)) / 2;

    if (send_network_caps(ctx, MODBUS_FC_WRITE_AND_READ_REGISTERS, addr, nb) == 0)
    {
        return modbus_write_and_read_registers(ctx, write_addr, write_nb, src,
                read_addr, read_nb, dest);
    }

    return -1;
}

/**
 * Shim for modbus_report_slave_id()
 *
 * 1. Sends a Macaroon with the MODBUS_FC_REPORT_SLAVE_ID command
 * 2. Send a request to get the slave ID of the device (only available in
 *    serial communication)
 * */
int modbus_report_slave_id_network_caps(modbus_t *ctx, int max_dest,
        uint8_t *dest)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    if (send_network_caps(ctx, MODBUS_FC_REPORT_SLAVE_ID, 0, 0) == 0)
    {
        return modbus_report_slave_id(ctx, max_dest, dest);
    }

    return -1;
}

/* Receive the request from a modbus master */
int modbus_receive_network_caps(modbus_t *ctx, uint8_t *req)
{
    return modbus_receive(ctx, req);
}

/******************
 * SERVER FUNCTIONS
 *****************/

int initialise_server_network_caps(modbus_t *ctx, const char *location, const char *key, const char *id)
{
    enum macaroon_returncode err = MACAROON_SUCCESS;

    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    key_ = (unsigned char *)key;
    key_sz_ = strnlen(key, MAX_MACAROON_INITIALISATION_LENGTH);

    id_ = (unsigned char *)id;
    id_sz_ = strnlen(id, MAX_MACAROON_INITIALISATION_LENGTH);

    location_ = (unsigned char *)location;
    location_sz_ = strnlen(location, MAX_MACAROON_INITIALISATION_LENGTH);

    server_macaroon_ = macaroon_create(location_, location_sz_,
            key_, key_sz_, id_, id_sz_, &err);
    if (err != MACAROON_SUCCESS)
    {
        if(modbus_get_debug(ctx)) {
            printf("Failed to initialise Macaroon\n");
            printf("err: %d\n", err);
        }
        return -1;
    }

    return 0;
}

/**
 * Process an incoming Macaroon:
 * 1. Deserialise a string
 * 2. Check if it's a valid Macaroon
 * 3. Perform verification on the Macaroon
 * */
static int process_network_caps(modbus_t *ctx, uint8_t *tab_string, int function, uint16_t addr, int nb)
{
    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    enum macaroon_returncode err = MACAROON_SUCCESS;

    unsigned char *serialised_macaroon;
    int serialised_macaroon_length;

    serialised_macaroon = (unsigned char *)tab_string;
    serialised_macaroon_length = strnlen((char *)serialised_macaroon, MODBUS_MAX_STRING_LENGTH);

    unsigned char *fc = create_function_caveat_from_fc(function);
    int function_as_caveat = 0;

    uint16_t ar_max = find_max_address(function, addr, nb);
    unsigned char *ar = create_address_caveat(addr, ar_max);
    int address_as_caveat = 0;

    struct macaroon *M = NULL;
    struct macaroon_verifier *V = macaroon_verifier_create();

    // try to deserialise the string into a Macaroon
    M = macaroon_deserialize(serialised_macaroon, serialised_macaroon_length, &err);

    if (err != MACAROON_SUCCESS)
    {
        if (modbus_get_debug(ctx))
        {
            printf("> Macaroon verification: FAIL\n");
            printf("> FAILED TO DESERIALISE\n");
            printf("%s\n", DISPLAY_MARKER);
        }
        return -1;
    }

    if (M == NULL)
    {
        if (modbus_get_debug(ctx))
        {
            printf("> Macaroon verification: MACAROON NOT INITIALISED\n");
            printf("%s\n", DISPLAY_MARKER);
        }
        return -1;
    }

    /**
     * - Confirm the fpcs aren't mutually exclusive (e.g., READ-ONLY and WRITE-ONLY)
     * - Confirm requested addresses are not out of range (based on caveats)
     * - Add all first party caveats to the verifier
     * - Confirm that the requested function is one of the first party caveats
     * - Confirm that the requested address range is one of the first party caveats
     * - Verify the Macaroon
     * */

    /* count fpcs */
    uint32_t num_fpcs = macaroon_num_first_party_caveats(M);
    if (num_fpcs > MAX_CAVEATS)
    {
        if (modbus_get_debug(ctx))
        {
            printf("> Macaroon verification: FAIL\n");
            printf("TOO MANY CAVEATS\n");
            printf("%s\n", DISPLAY_MARKER);
        }
        return -1;
    }

    /* extract fpcs */
    unsigned char *fpcs[MAX_CAVEATS];
    const unsigned char *fpc;
    size_t fpc_sz;
    for (size_t i = 0; i < num_fpcs; ++i)
    {
        macaroon_first_party_caveat(M, i, &fpc, &fpc_sz);
#if defined(__freertos__)
        fpcs[i] = (unsigned char *)pvPortMalloc((fpc_sz + 1) * sizeof(unsigned char));
#else
        fpcs[i] = (unsigned char *)malloc((fpc_sz + 1) * sizeof(unsigned char));
#endif
        memset(fpcs[i], 0, (fpc_sz + 1) * sizeof(unsigned char));
        strncpy((char *)fpcs[i], (char *)fpc, fpc_sz);
    }

    // functions: perform mutual exclusion check
    if (check_function_caveats(fpcs, num_fpcs) != 0)
    {
        if (modbus_get_debug(ctx))
        {
            printf("> Function caveats are mutually exclusive\n");
            printf("%s\n", DISPLAY_MARKER);
        }
        return -1;
    }

    // addresses: perform range check
    if (check_address_caveats(fpcs, num_fpcs, ar) != 0)
    {
        if (modbus_get_debug(ctx))
        {
            printf("> Requested addresses are out of range\n");
            printf("%s\n", DISPLAY_MARKER);
        }
        return -1;
    }

    /* add fpcs to the verifier */
    for (size_t i = 0; i < num_fpcs; ++i)
    {
        // add fpcs to verifier
        macaroon_verifier_satisfy_exact(
                V, (const unsigned char *)fpcs[i],
                strnlen((char *)fpcs[i], MAX_CAVEAT_LENGTH), &err);

        if (err != MACAROON_SUCCESS)
        {
            if (modbus_get_debug(ctx))
            {
                printf("> Failed to add caveat to verifier\n");
                printf("%s\n", DISPLAY_MARKER);
            }
            return -1;
        }

        /* check if the requested function and address range are caveats */
        if (strncmp((char *)fpcs[i], (char *)fc, MAX_CAVEAT_LENGTH) == 0)
        {
            function_as_caveat = 1;
        }
        else if (strncmp((char *)fpcs[i], (char *)ar, MAX_CAVEAT_LENGTH) == 0)
        {
            address_as_caveat = 1;
        }

#if defined(__freertos__)
        vPortFree(fpcs[i]);
#else
        free(fpcs[i]);
#endif
    }

#if defined(__freertos__)
    vPortFree(fc);
    vPortFree(ar);
#else
    free(fc);
    free(ar);
#endif

    // confirm the requested function is a caveat
    if (!function_as_caveat)
    {
        if (modbus_get_debug(ctx))
        {
            printf("> Function not protected as a Macaroon caveat\n");
            printf("%s\n", DISPLAY_MARKER);
        }
        return -1;
    }

    // confirm the requested addresses is a caveat
    if (!address_as_caveat)
    {
        if (modbus_get_debug(ctx))
        {
            printf("> Address range not protected as a Macaroon caveat\n");
            printf("%s\n", DISPLAY_MARKER);
        }
        return -1;
    }

    // perform verification
    macaroon_verify(V, M, key_, key_sz_, NULL, 0, &err);
    if (err != MACAROON_SUCCESS)
    {
        if (modbus_get_debug(ctx))
        {
            printf("> Macaroon verification: FAIL\n");
            printf("%s\n", DISPLAY_MARKER);
        }

        return -1;
    }

    if (modbus_get_debug(ctx))
    {
        printf("> Macaroon verification: PASS\n");
        printf("%s\n", DISPLAY_MARKER);
    }

    macaroon_destroy(M);
    macaroon_verifier_destroy(V);
    return 0;
}

/**
 * Performs Macaroons-related preprocessing of a request
 *
 * E.g., Macaroon verification or zeroing the state string
 * that holds the Macaroon.
 * */
int modbus_preprocess_request_network_caps(modbus_t *ctx, uint8_t *req, modbus_mapping_t *mb_mapping)
{
    int offset;
    int slave_id;
    int function;
    uint16_t addr;
    int nb;
    uint16_t addr_wr;
    int nb_wr;

    if (modbus_get_debug(ctx))
    {
        print_shim_info("macaroons_shim", __FUNCTION__);
    }

    /* get the function, nb, and addr information from the request */
    modbus_decompose_request(ctx, req, &offset, &slave_id, &function, &addr, &nb, &addr_wr, &nb_wr);

    if (modbus_get_debug(ctx))
    {
        print_modbus_decompose_request(ctx, req);
        printf("\n");
        print_mb_mapping(mb_mapping);
    }

    /**
     * If the function is WRITE_STRING we reset tab_string
     * If the function is READ_STRING, serialise the server Macaroon and store in tab_string
     * If the function is anything else, we verify the Macaroon
     * */
    switch (function)
    {
        case MODBUS_FC_WRITE_STRING:
            {
                /**
                 * Zero out the state variable where the Macaroon string is stored
                 * then continue to process the request
                 * */
                memset(mb_mapping->tab_string, 0, MODBUS_MAX_STRING_LENGTH * sizeof(uint8_t));

                if (modbus_get_debug(ctx))
                {
                    printf("%s\n", DISPLAY_MARKER);
                }
            }
            break;

        case MODBUS_FC_READ_STRING:
            {
                enum macaroon_returncode err = MACAROON_SUCCESS;

                size_t serialised_macaroon_length;
                unsigned char *serialised_macaroon;

                /**
                 * Serialise the server Macaroon and feed it into tab_string
                 * then continue to process the request
                 * */
                if (server_macaroon_ != NULL)
                {
                    serialised_macaroon_length = macaroon_serialize_size_hint(server_macaroon_, MACAROON_V1);
#if defined(__freertos__)
                    serialised_macaroon = (unsigned char *)pvPortMalloc(serialised_macaroon_length * sizeof(unsigned char));
#else
                    serialised_macaroon = (unsigned char *)malloc(serialised_macaroon_length * sizeof(unsigned char));
#endif

                    macaroon_serialize(server_macaroon_, MACAROON_V1,
                            serialised_macaroon, serialised_macaroon_length, &err);

                    if (err != MACAROON_SUCCESS)
                    {
                        printf("err: %d\n", err);
                        return -1;
                    }
                    strncpy((char *)mb_mapping->tab_string, (char *)serialised_macaroon, serialised_macaroon_length);
#if defined(__freertos__)
                    vPortFree(serialised_macaroon);
#else
                    free(serialised_macaroon);
#endif
                }

                if (modbus_get_debug(ctx))
                {
                    printf("%s\n", DISPLAY_MARKER);
                }
            }
            break;

        default:
            {
                /**
                 * process_network_caps() needs an address range, which is tricky
                 * for write_and_read_registers, since it has two ranges
                 *
                 * we need to find the entire range that the function is trying to access
                 * since there's no way to handle disjoint caveats
                 *
                 * based on modbus.c, addr = write_addr, nb = write_nb, addr_wr = read_addr, nb_wr = read_nb
                 * */
                if (function == MODBUS_FC_WRITE_AND_READ_REGISTERS)
                {
                    uint16_t write_addr = addr;
                    uint16_t read_addr = addr_wr;
                    int write_nb = nb;
                    int read_nb = nb_wr;
                    uint16_t write_addr_max = find_max_address(MODBUS_FC_WRITE_AND_READ_REGISTERS, write_addr, write_nb);
                    uint16_t read_addr_max = find_max_address(MODBUS_FC_WRITE_AND_READ_REGISTERS, read_addr, read_nb);
                    addr = (write_addr < read_addr) ? write_addr : read_addr;
                    nb = ((write_addr < read_addr) ? (read_addr_max - write_addr) : (write_addr_max - read_addr)) / 2;
                }

                /**
                 * Extract the previously-received Macaroon
                 * If verification fails, return -1
                 * */
                if (process_network_caps(ctx, mb_mapping->tab_string, function, addr, nb) != 0)
                {
                    return -1;
                }
            }
    }

    return 0;
}

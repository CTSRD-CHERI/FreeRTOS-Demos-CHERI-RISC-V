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

#include "modbus_object_caps.h"

/*********
 * GLOBALS
 ********/

/**
 * Create copies of pointers to mb_mapping and members for restoration
 * after calling libmodbus:modbus_process_request.
 *
 * This allows reducing permissions to the structure and members before sending
 * them to libmodbus:modbus_process_request.
 * */
#if defined(MODBUS_OBJECT_CAPS)
static uint8_t *tab_bits_;
static uint8_t *tab_input_bits_;
static uint16_t *tab_input_registers_;
static uint16_t *tab_registers_;
static uint8_t *tab_string_;
static modbus_mapping_t *mb_mapping_;
#endif

/******************
 * SERVER FUNCTIONS
 *****************/
/**
 * Shim function for libmodbus:modbus_mapping_new_start_address
 *
 * modbus_mapping_new_start_address allocates memory and establishes
 * the initial server state wrt coils and registers
 *
 * This shim reduces the permissions on the pointer to structure s.t.
 * pointer structure members can only load and store data, and the
 * pointer to the structure itself can only load and store the data
 * and pointers within the structure
 * */
modbus_mapping_t* modbus_mapping_new_start_address_object_caps(
    modbus_t *ctx,
    unsigned int start_bits, unsigned int nb_bits,
    unsigned int start_input_bits, unsigned int nb_input_bits,
    unsigned int start_registers, unsigned int nb_registers,
    unsigned int start_input_registers, unsigned int nb_input_registers)
{
    if(modbus_get_debug(ctx)) {
        print_shim_info("object capabilities shim", __FUNCTION__);
    }

    modbus_mapping_t* mb_mapping;
    mb_mapping = modbus_mapping_new_start_address(
        start_bits, nb_bits,
        start_input_bits, nb_input_bits,
        start_registers, nb_registers,
        start_input_registers, nb_input_registers);

#if defined(MODBUS_OBJECT_CAPS)
    // may need to be able to read and write to coils
    mb_mapping->tab_bits = (uint8_t *)cheri_perms_and(mb_mapping->tab_bits, CHERI_PERM_LOAD | CHERI_PERM_STORE);
    tab_bits_ = mb_mapping->tab_bits;

    // generally, only need to read discrete inputs; however, need to write to initialise
    mb_mapping->tab_input_bits = (uint8_t *)cheri_perms_and(mb_mapping->tab_input_bits, CHERI_PERM_LOAD | CHERI_PERM_STORE);
    tab_input_bits_ = mb_mapping->tab_input_bits;

    // generally, only need to read input registers; however, need to write to initialise
    mb_mapping->tab_input_registers = (uint16_t *)cheri_perms_and(mb_mapping->tab_input_registers, CHERI_PERM_LOAD | CHERI_PERM_STORE);
    tab_input_registers_ = mb_mapping->tab_input_registers;

    // may need to read and write to holding registers
    mb_mapping->tab_registers = (uint16_t *)cheri_perms_and(mb_mapping->tab_registers, CHERI_PERM_LOAD | CHERI_PERM_STORE);
    tab_registers_ = mb_mapping->tab_registers;

    // may need to read and write to the string (used for Macaroons)
    mb_mapping->tab_string = (uint8_t *)cheri_perms_and(mb_mapping->tab_string, CHERI_PERM_LOAD | CHERI_PERM_STORE);
    tab_string_ = mb_mapping->tab_string;

    mb_mapping_ = mb_mapping;
#endif

    return mb_mapping;
}

/**
 * Shim function for libmodbus:modbus_process_request
 *
 * preprocesses a client request, modifies the server
 * state (if applicable) and returns to the caller
 * */
int modbus_preprocess_request_object_caps(modbus_t *ctx, uint8_t *req, modbus_mapping_t *mb_mapping)
{
    int debug = modbus_get_debug(ctx);

    if(debug) {
        print_shim_info("object capabilities shim", __FUNCTION__);
        printf("\n");
    }

#if defined(MODBUS_OBJECT_CAPS)
    /* need to be able to STORE to modify mb_mapping permissions */
    mb_mapping = (modbus_mapping_t *)cheri_perms_and(mb_mapping_,
        CHERI_PERM_STORE | CHERI_PERM_STORE_CAP | CHERI_PERM_STORE_LOCAL_CAP);

    /* reduce most table permissions to zero, then restore them per the function */
    mb_mapping->tab_bits = NULL;
    mb_mapping->tab_input_bits = NULL;
    mb_mapping->tab_input_registers = NULL;
    mb_mapping->tab_registers = NULL;

    /* for Macaroons, set tab_string permissions to LOAD, otherwise to zero */
#if defined(MODBUS_NETWORK_CAPS)
    mb_mapping->tab_string = (uint8_t *)cheri_perms_and(tab_string_, CHERI_PERM_LOAD);
#else
    mb_mapping->tab_string = NULL;
#endif

    /* reduce mb_mapping capabilities based on the function in the request */
    switch (modbus_get_function_code(ctx, req))
    {
    case MODBUS_FC_READ_COILS:
    {
        /* we need to be able to read coil (tab_bits) values */
        mb_mapping->tab_bits = (uint8_t *)cheri_perms_and(tab_bits_, CHERI_PERM_LOAD);

        /* we need to be able to read macaroon (tab_string) values */
        mb_mapping->tab_string = (uint8_t *)cheri_perms_and(tab_string_, CHERI_PERM_LOAD);

        /* revert mb_mapping to LOAD only */
        mb_mapping = (modbus_mapping_t *)cheri_perms_and(mb_mapping_, CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP);
    }
    break;

    case MODBUS_FC_READ_DISCRETE_INPUTS:
    {
        /* we only need to be able to read discrete inputs (tab_input_bits) */
        mb_mapping->tab_input_bits = (uint8_t *)cheri_perms_and(tab_input_bits_, CHERI_PERM_LOAD);

        /* we need to be able to read macaroon (tab_string) values */
        mb_mapping->tab_string = (uint8_t *)cheri_perms_and(tab_string_, CHERI_PERM_LOAD);

        /* revert mb_mapping to LOAD only */
        mb_mapping = (modbus_mapping_t *)cheri_perms_and(mb_mapping_, CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP);
    }
    break;

    case MODBUS_FC_READ_HOLDING_REGISTERS:
    {
        /* we only need to be able to read holding registers (tab_registers) */
        mb_mapping->tab_registers = (uint16_t *)cheri_perms_and(tab_registers_, CHERI_PERM_LOAD);

        /* we need to be able to read macaroon (tab_string) values */
        mb_mapping->tab_string = (uint8_t *)cheri_perms_and(tab_string_, CHERI_PERM_LOAD);

        /* revert mb_mapping to LOAD only */
        mb_mapping = (modbus_mapping_t *)cheri_perms_and(mb_mapping_, CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP);
    }
    break;

    case MODBUS_FC_READ_INPUT_REGISTERS:
    {
        /* we only need to be able to read input registers (tab_input_registers) */
        mb_mapping->tab_input_registers = (uint16_t *)cheri_perms_and(tab_input_registers_, CHERI_PERM_LOAD);

        /* we need to be able to read macaroon (tab_string) values */
        mb_mapping->tab_string = (uint8_t *)cheri_perms_and(tab_string_, CHERI_PERM_LOAD);

        /* revert mb_mapping to LOAD only */
        mb_mapping = (modbus_mapping_t *)cheri_perms_and(mb_mapping_, CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP);
    }
    break;

    case MODBUS_FC_WRITE_SINGLE_COIL:
    case MODBUS_FC_WRITE_MULTIPLE_COILS:
    {
        /* we only need to be able to store coil (tab_bits) values */
        mb_mapping->tab_bits = (uint8_t *)cheri_perms_and(tab_bits_, CHERI_PERM_STORE);

        /* we need to be able to read macaroon (tab_string) values */
        mb_mapping->tab_string = (uint8_t *)cheri_perms_and(tab_string_, CHERI_PERM_LOAD);

        /* revert mb_mapping to LOAD only */
        mb_mapping = (modbus_mapping_t *)cheri_perms_and(mb_mapping_, CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP);
    }
    break;

    case MODBUS_FC_WRITE_SINGLE_REGISTER:
    case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
    {
        /* we only need to be able to write holding registers (tab_registers) */
        mb_mapping->tab_registers = (uint16_t *)cheri_perms_and(tab_registers_, CHERI_PERM_STORE);

        /* we need to be able to read macaroon (tab_string) values */
        mb_mapping->tab_string = (uint8_t *)cheri_perms_and(tab_string_, CHERI_PERM_LOAD);

        /* revert mb_mapping to LOAD only */
        mb_mapping = (modbus_mapping_t *)cheri_perms_and(mb_mapping_, CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP);
    }
    break;

    case MODBUS_FC_REPORT_SLAVE_ID:
        /* we don't need access to mb_modbus */
        mb_mapping = NULL;
        break;

    case MODBUS_FC_MASK_WRITE_REGISTER:
    case MODBUS_FC_WRITE_AND_READ_REGISTERS:
    {
        /* we only need to be able to read and write holding registers (tab_registers) */
        mb_mapping->tab_registers = (uint16_t *)cheri_perms_and(tab_registers_, CHERI_PERM_LOAD | CHERI_PERM_STORE);

        /* we need to be able to read macaroon (tab_string) values */
        mb_mapping->tab_string = (uint8_t *)cheri_perms_and(tab_string_, CHERI_PERM_LOAD);

        /* revert mb_mapping to LOAD only */
        mb_mapping = (modbus_mapping_t *)cheri_perms_and(mb_mapping_, CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP);
    }
    break;

    case MODBUS_FC_WRITE_STRING:
    case MODBUS_FC_READ_STRING:
    {
        /* we only need to be able to read and write the string (tab_string) used for Macaroons */
        mb_mapping->tab_string = (uint8_t *)cheri_perms_and(tab_string_, CHERI_PERM_LOAD | CHERI_PERM_STORE);

        /* revert mb_mapping to LOAD only */
        mb_mapping = (modbus_mapping_t *)cheri_perms_and(mb_mapping_, CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP);
    }
    break;

    default:
        /* set mb_mapping to NULL */
        mb_mapping = NULL;
    }
#endif

    if(debug) {
        /* Print the decomposed request and the resulting mb_mapping pointers */
        print_modbus_decompose_request(ctx, req);
        printf("\n");
        print_mb_mapping(mb_mapping);
        printf("\n");
    }

    return 0;
}

/**
 * This is a (mostly) empty function used to measure the overhead of calling into this library
 * */
int modbus_preprocess_request_object_caps_stub(modbus_t *ctx, uint8_t *req, modbus_mapping_t *mb_mapping)
{
    int debug = modbus_get_debug(ctx);

    if(debug) {
        print_shim_info("object capabilities shim", __FUNCTION__);
        printf("\n");

        /* Print the decomposed request and the resulting mb_mapping pointers */
        print_modbus_decompose_request(ctx, req);
        printf("\n");
        print_mb_mapping(mb_mapping);
    }

    return 0;
}

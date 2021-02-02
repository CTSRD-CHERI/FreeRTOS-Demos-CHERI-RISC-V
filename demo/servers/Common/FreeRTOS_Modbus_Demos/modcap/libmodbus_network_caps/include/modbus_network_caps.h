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

#ifndef _MODBUS_NETWORK_CAPS_H_
#define _MODBUS_NETWORK_CAPS_H_

#include <stdio.h>
#include <string.h>

/* for Modbus */
#include <modbus/modbus.h>
#include <modbus/modbus-helpers.h>

/* Macaroons */
#include "macaroons/macaroons.h"

#if defined(__freertos__)
/* FreeRTOS */
#include "FreeRTOS.h"
#endif

/*************
 * DEFINITIONS
 ************/

/******************
 * SERVER FUNCTIONS
 *****************/
int initialise_server_network_caps(modbus_t *ctx, const char *location, const char *key, const char *id);
int modbus_receive_network_caps(modbus_t *ctx, uint8_t *req);
int modbus_preprocess_request_network_caps(modbus_t *ctx, uint8_t *req, modbus_mapping_t *mb_mapping);

/******************
 * CLIENT FUNCTIONS
 *****************/
int initialise_client_network_caps(modbus_t *ctx, char *serialised_macaroon, int serialised_macaroon_length);
int modbus_read_bits_network_caps(modbus_t *ctx, int addr, int nb, uint8_t *dest);
int modbus_read_input_bits_network_caps(modbus_t *ctx, int addr, int nb, uint8_t *dest);
int modbus_read_registers_network_caps(modbus_t *ctx, int addr, int nb, uint16_t *dest);
int modbus_read_input_registers_network_caps(modbus_t *ctx, int addr, int nb, uint16_t *dest);
int modbus_write_bit_network_caps(modbus_t *ctx, int addr, int status);
int modbus_write_register_network_caps(modbus_t *ctx, int addr, const uint16_t value);
int modbus_write_bits_network_caps(modbus_t *ctx, int addr, int nb, const uint8_t *src);
int modbus_write_registers_network_caps(modbus_t *ctx, int addr, int nb, const uint16_t *data);
int modbus_mask_write_register_network_caps(modbus_t *ctx, int addr, uint16_t and_mask, uint16_t or_mask);
int modbus_write_and_read_registers_network_caps(modbus_t *ctx, int write_addr, int write_nb,
                                               const uint16_t *src, int read_addr, int read_nb,
                                               uint16_t *dest);
int modbus_report_slave_id_network_caps(modbus_t *ctx, int max_dest, uint8_t *dest);

#endif /* _MODBUS_NETWORK_CAPS_H_ */

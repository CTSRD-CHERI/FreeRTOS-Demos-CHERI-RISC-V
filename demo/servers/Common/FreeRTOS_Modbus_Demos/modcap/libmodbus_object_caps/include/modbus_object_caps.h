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

#ifndef _MODBUS_OBJECT_CAPABILITIES_H_
#define _MODBUS_OBJECT_CAPABILITIES_H_

#include <stdio.h>

/* For CHERI */
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#else
#warning libmodbus_object_capabilities only provides a pass-through if CHERI capabilities are not supported
#endif

/* for modbus */
#include <modbus/modbus.h>
#include <modbus/modbus-helpers.h>

/**
 *  Allocates 5 arrays to store bits, input bits, registers, inputs
 * registers, and a string. The pointers are stored in modbus_mapping structure.
 *
 * The modbus_mapping_new_start_address() function shall return the new allocated
 * structure if successful. Otherwise it shall return NULL and set errno to
 * ENOMEM.
 * */
modbus_mapping_t* modbus_mapping_new_start_address_object_caps(
    modbus_t *ctx,
    unsigned int start_bits, unsigned int nb_bits,
    unsigned int start_input_bits, unsigned int nb_input_bits,
    unsigned int start_registers, unsigned int nb_registers,
    unsigned int start_input_registers, unsigned int nb_input_registers);

int modbus_preprocess_request_object_caps(modbus_t *ctx, uint8_t *req, modbus_mapping_t *mb_mapping);
int modbus_preprocess_request_object_caps(modbus_t *ctx, uint8_t *req, modbus_mapping_t *mb_mapping);

#endif /* _MODBUS_OBJECT_CAPABILITIES_H_ */

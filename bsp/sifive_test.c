/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Hesham Almatary <Hesham.Almatary@cl.cam.ac.uk>
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
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

#ifndef _RISCV_SIFIVE_TEST_H
#define _RISCV_SIFIVE_TEST_H

#include <inttypes.h>
#include <stddef.h>

#ifndef __waf__
#include <bsp/qemu_virt.h>
#endif

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheric.h>
extern void *pvAlmightyDataCap;
#endif

#define SIFIVE_TEST_BASE  0x100000
#define TEST_PASS         0x5555
#define TEST_FAIL         0x3333

void vTerminate( int32_t lExitCode )
{
volatile uint32_t *sifive_test = ( uint32_t * ) SIFIVE_TEST_BASE;
uint32_t test_command = TEST_PASS;

	#ifdef __CHERI_PURE_CAPABILITY__
		sifive_test = cheri_setoffset( pvAlmightyDataCap, ( ptraddr_t ) sifive_test );
		sifive_test = cheri_csetbounds( ( void * ) sifive_test, sizeof( uint32_t ) );
	#endif

	if( lExitCode != 0 )
	{
		test_command = ( lExitCode << 16 ) | TEST_FAIL;
	}

	while( sifive_test != NULL )
	{
		*sifive_test =  test_command;
	}
}

#endif /* _RISCV_SIFIVE_TEST_H */

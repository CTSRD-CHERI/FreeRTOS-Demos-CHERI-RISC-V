/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _UNIT_TEST_H_
#define _UNIT_TEST_H_

#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# ifndef _MSC_VER
# include <stdint.h>
# else
# include "stdint.h"
# endif
#endif

#define SERVER_ID         17
#define INVALID_SERVER_ID 18

#define TEST_STRING                                                       \
   "[4D][44][41][79][5A]"                                                 \
   "[6D][78][76][59][32][46][30][61][57][39][75][49][47][68][30][64][48]" \
   "[42][7A][4F][69][38][76][64][33][64][33][4C][6D][31][76][5A][47][4A]" \
   "[31][63][79][35][6A][62][32][30][76][62][57][46][6A][59][58][4A][76]" \
   "[62][32][35][7A][4C][77][6F][77][4D][44][49][7A][61][57][52][6C][62]" \
   "[6E][52][70][5A][6D][6C][6C][63][69][42][70][5A][43][42][6D][62][33]" \
   "[49][67][59][53][42][69][59][57][51][67][63][32][56][6A][63][6D][56]" \
   "[30][43][6A][41][77][4D][54][5A][6A][61][57][51][67][5A][6E][56][75]" \
   "[59][33][52][70][62][32][34][67][50][53][41][7A][4D][67][6F][77][4D]" \
   "[44][46][69][59][32][6C][6B][49][47][46][6B][5A][48][4A][6C][63][33]" \
   "[4D][67][50][53][41][78][4F][54][6B][79][4D][7A][49][30][4F][41][6F]" \
   "[77][4D][44][4A][6D][63][32][6C][6E][62][6D][46][30][64][58][4A][6C]" \
   "[49][4E][50][45][6B][72][6F][63][42][30][6C][5A][56][38][48][61][48]" \
   "[56][44][61][66][78][39][5A][77][33][5F][78][67][52][48][4D][67][79]" \
   "[41][65][4B][69][44][70][65][49][64][6A][43][67]"

const uint16_t UT_BITS_ADDRESS = 0x130;
const uint16_t UT_BITS_NB = 0x25;
const uint8_t UT_BITS_TAB[] = { 0xCD, 0x6B, 0xB2, 0x0E, 0x1B };

const uint16_t UT_INPUT_BITS_ADDRESS = 0x1C4;
const uint16_t UT_INPUT_BITS_NB = 0x16;
const uint8_t UT_INPUT_BITS_TAB[] = { 0xAC, 0xDB, 0x35 };

const uint16_t UT_REGISTERS_ADDRESS = 0x160;
const uint16_t UT_REGISTERS_NB = 0x3;
const uint16_t UT_REGISTERS_NB_MAX = 0x20;
const uint16_t UT_REGISTERS_TAB[] = { 0x022B, 0x0001, 0x0064 };

/* Raise a manual exception when this address is used for the first byte */
const uint16_t UT_REGISTERS_ADDRESS_SPECIAL = 0x170;
/* The response of the server will contains an invalid TID or slave */
const uint16_t UT_REGISTERS_ADDRESS_INVALID_TID_OR_SLAVE = 0x171;
/* The server will wait for 1 second before replying to test timeout */
const uint16_t UT_REGISTERS_ADDRESS_SLEEP_500_MS = 0x172;
/* The server will wait for 5 ms before sending each byte */
const uint16_t UT_REGISTERS_ADDRESS_BYTE_SLEEP_5_MS = 0x173;

/* If the following value is used, a bad response is sent.
   It's better to test with a lower value than
   UT_REGISTERS_NB_POINTS to try to raise a segfault. */
const uint16_t UT_REGISTERS_NB_SPECIAL = 0x2;

const uint16_t UT_INPUT_REGISTERS_ADDRESS = 0x108;
const uint16_t UT_INPUT_REGISTERS_NB = 0x1;
const uint16_t UT_INPUT_REGISTERS_TAB[] = { 0x000A };

const float UT_REAL = 123456.00;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
const uint32_t UT_IREAL_ABCD = 0x47F12000;
const uint32_t UT_IREAL_DCBA = 0x0020F147;
const uint32_t UT_IREAL_BADC = 0xF1470020;
const uint32_t UT_IREAL_CDAB = 0x200047F1;
#else
const uint32_t UT_IREAL_ABCD = 0x0020F147;
const uint32_t UT_IREAL_DCBA = 0x47F12000;
const uint32_t UT_IREAL_BADC = 0x200047F1;
const uint32_t UT_IREAL_CDAB = 0xF1470020;
#endif

#endif /* _UNIT_TEST_H_ */

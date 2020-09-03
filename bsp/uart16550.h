// See LICENSE for license details.

#ifndef _RISCV_16550_H
#define _RISCV_16550_H

#include <stdint.h>
#include <FreeRTOSConfig.h>

#if configUART16550_REGSHIFT == 1
extern volatile uint8_t* uart16550;
#elif configUART16550_REGSHIFT == 2
extern volatile uint32_t* uart16550;
#else
#error "Unsupported uart reg_shift value"
#endif

void uart16550_putchar(uint8_t ch);
int uart16550_getchar(void);
int uart16550_txbuffer(uint8_t *ptr, int len);
void uart16550_init(unsigned long base);

#endif

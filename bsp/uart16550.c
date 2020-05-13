// See LICENSE for license details.

#include <string.h>
#include "uart16550.h"

#ifdef CONFIG_ENABLE_CHERI
#include <cheric.h>
#endif /* CONFIG_ENABLE_CHERI */

volatile uint8_t* uart16550;

#define UART_REG_QUEUE     0
#define UART_REG_LINESTAT  5
#define UART_REG_STATUS_RX 0x01
#define UART_REG_STATUS_TX 0x20

static inline uint32_t bswap(uint32_t x) {
  uint32_t y = (x & 0x00FF00FF) <<  8 | (x & 0xFF00FF00) >>  8;
  uint32_t z = (y & 0x0000FFFF) << 16 | (y & 0xFFFF0000) >> 16;
  return z;
}

void uart16550_putchar(uint8_t ch) {
//  while ((uart16550[UART_REG_LINESTAT] & UART_REG_STATUS_TX) == 0);
  uart16550[UART_REG_QUEUE] = ch;
}

int uart16550_getchar() {
  if (uart16550[UART_REG_LINESTAT] & UART_REG_STATUS_RX)
    return uart16550[UART_REG_QUEUE];
  return -1;
}

int uart16550_txbuffer(uint8_t *ptr, int len) {

  for (int i = 0; i < len; i++) {
    uart16550_putchar(ptr[i]);
  }

  return len;
}

void uart16550_init(unsigned long base) {
  uart16550 = (void *) base;

#ifdef CONFIG_ENABLE_CHERI
  extern void *pvAlmightyDataCap;
  uart16550 = (uintptr_t) cheri_setoffset(pvAlmightyDataCap, (size_t) base);
  uart16550 = cheri_csetbounds(uart16550, 0x1000);
#endif /* CONFIG_ENABLE_CHERI */

  // http://wiki.osdev.org/Serial_Ports
  /*
  uart16550[1] = 0x00;    // Disable all interrupts
  uart16550[3] = 0x80;    // Enable DLAB (set baud rate divisor)
  uart16550[0] = 0x03;    // Set divisor to 3 (lo byte) 38400 baud
  uart16550[1] = 0x00;    //                  (hi byte)
  uart16550[3] = 0x03;    // 8 bits, no parity, one stop bit
  uart16550[2] = 0xC7;    // Enable FIFO, clear them, with 14-byte threshold
  */
}

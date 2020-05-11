#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "htif.h"

/* Most of the code below is copied from riscv-pk project */
# define TOHOST_CMD(dev, cmd, payload) \
  (((uint64_t)(dev) << 56) | ((uint64_t)(cmd) << 48) | (uint64_t)(payload))

#define FROMHOST_DEV(fromhost_value) ((uint64_t)(fromhost_value) >> 56)
#define FROMHOST_CMD(fromhost_value) ((uint64_t)(fromhost_value) << 8 >> 56)
#define FROMHOST_DATA(fromhost_value) ((uint64_t)(fromhost_value) << 16 >> 16)

volatile uint64_t tohost __attribute__((section(".htif"))) __attribute__ ((aligned (8)));
volatile uint64_t fromhost __attribute__((section(".htif"))) __attribute__ ((aligned (8)));
volatile uint64_t riscv_fill_up_htif_section[510] __attribute__((section(".htif")));
volatile int htif_console_buf;

static void __check_fromhost(void) {
  uint64_t fh = fromhost;
  if (!fh) {
    return;
  }
  fromhost = 0;

  // this should be from the console
  assert(FROMHOST_DEV(fh) == 1);
  switch (FROMHOST_CMD(fh)) {
  case 0:
    htif_console_buf = 1 + (uint8_t)FROMHOST_DATA(fh);
    break;
  case 1:
    break;
  default:
    assert(0);
  }
}

static void __set_tohost(uintptr_t dev, uintptr_t cmd, uintptr_t data) {
  while (tohost) {
    __check_fromhost();
  }
  tohost = TOHOST_CMD(dev, cmd, data);
}

int htif_console_getchar(void) {
  __check_fromhost();
  int ch = htif_console_buf;
  if (ch >= 0) {
    htif_console_buf = -1;
    __set_tohost(1, 0, 0);
  }

  return ch - 1;
}

void htif_console_putchar(char c) {
  __set_tohost(1, 1, c);
}

int htif_console_write_polled(
  const char *buf,
  size_t len
) {
  size_t i;

  for (i = 0; i < len; ++i) {
    htif_console_putchar(buf[i]);
  }

  return i;
}

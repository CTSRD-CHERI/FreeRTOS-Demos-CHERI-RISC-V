#ifndef RISCV_RVBS_PLATFORM_H
#define RISCV_RVBS_PLATFORM_H

#define configCLINT_BASE_ADDRESS 0x2000000
#define configRVBS_PUTCHAR_ADDRESS 0x10000000

static inline int plat_console_write(uint8_t *ptr, int len) {

  for (int i = 0; i < len; i++) {
    *((volatile char *) configRVBS_PUTCHAR_ADDRESS) = ptr[i];
  }

  return len;
}

#endif /* RISCV_RVBS_PLATFORM_H */

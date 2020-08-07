#ifndef _RISCV_HTIF_H
#define _RISCV_HTIF_H

void htif_console_putchar(char);
int htif_console_getchar(void);
void htif_poweroff(int32_t lExitCode) __attribute__((noreturn));
int htif_console_write_polled(const char *buf, size_t len);
#endif

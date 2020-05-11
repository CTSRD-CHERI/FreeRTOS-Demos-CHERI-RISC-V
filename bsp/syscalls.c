#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>
#include "bsp.h"
#include "htif.h"
#include "uart16550.h"

void *_sbrk(int nbytes);
int _write(int file, char *ptr, int len);
int _close(int fd);
int _fstat(int fd, void *buffer);
long _lseek(int fd, long offset, int origin);
int _read(int fd, void *buffer, unsigned int count);
int _isatty(int fd);
int _kill(int pid, int sig);
int _getpid(int n);

void *_sbrk(int nbytes) {
  (void)nbytes;
  errno = ENOMEM;
  return (void *) -1;
}

int _write(int file, char *ptr, int len) {
  (void)file;
#if PLATFORM_SPIKE || PLATFORM_SAIL
  return htif_console_write_polled(ptr, len);
#elif PLATFORM_QEMU_VIRT || PLATFORM_PICCOLO
  return uart16550_txbuffer((uint8_t *) ptr, len);
#elif PLATFORM_RVBS
  return plat_console_write(ptr, len);
#else
#error "Unsupported Console for this PLATFORM"
#endif
}

int _close(int fd) {
  (void)fd;
  errno = EBADF;
  return -1;
}

long _lseek(int fd, long offset, int origin) {
  (void)fd;
  (void)offset;
  (void)origin;
  errno = EBADF;
  return -1;
}

int _read(int fd, void *buffer, unsigned int count) {
  (void)fd;
  (void)buffer;
  (void)count;
  errno = EBADF;
  return -1;
}

int _fstat(int fd, void *buffer) {
  (void)fd;
  (void)buffer;
  errno = EBADF;
  return -1;
}

int _isatty(int fd) {
  (void)fd;
  errno = EBADF;
  return 0;
}

int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  errno = EBADF;
  return -1;
}

int _getpid(int n) {
  (void)n;
  return 1;
}

void _exit(int x) {
  (void)x;
  do {

  } while (1);

}

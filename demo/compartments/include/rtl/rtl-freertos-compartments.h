#if !defined (_FREERTOS_RTL_COMPARTMENTS_H_)
#define _FREERTOS_RTL_COMPARTMENTS_H_

#include <FreeRTOS.h>
#include <stdbool.h>
#include <rtl/rtl-obj.h>

typedef struct compartment {
  void*       cap;
  void**      cap_list;
  char*       name;
  size_t      size;
  // TCB?
  // Symtab?
} compartment_t;

extern compartment_t comp_list[configCOMPARTMENTS_NUM];
extern char comp_strtab[configCOMPARTMENTS_NUM][configMAXLEN_COMPNAME];

int rtl_freertos_compartment_open(const char *name);
ssize_t rtl_freertos_compartment_read(int fd, void *buffer, UBaseType_t offset, size_t count);
size_t rtl_freertos_compartment_getsize(int fd);
size_t rtl_freertos_global_symbols_add(rtems_rtl_obj* obj);
#endif

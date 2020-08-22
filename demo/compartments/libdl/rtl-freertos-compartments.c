#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rtl/rtl-freertos-compartments.h>
#include <rtl/rtl-allocator.h>
#include <errno.h>

#include <FreeRTOS.h>

compartment_t comp_list[configCOMPARTMENTS_NUM];

int rtl_freertos_compartment_open(const char *name)
{
  const char* comp_name = NULL;
  char name_buff[configMAXLEN_COMPNAME];

  char *name_dot = strrchr(name, '.');
  if (name_dot == NULL) {
    return -1;
  }

  comp_name = rtems_rtl_alloc_new (RTEMS_RTL_ALLOC_OBJECT, name_dot - name + 1, true);
  if (!comp_name) {
    rtems_rtl_set_error (ENOMEM, "no memory for comp file name");
    return -1;
  }

  memcpy(comp_name, name, name_dot - name);

  for(int i = 0; i < configCOMPARTMENTS_NUM; i++) {
    if (strncmp(comp_list[i].name, comp_name, configMAXLEN_COMPNAME) == 0) {
      return i;
    }
  }

  return -1;
}

ssize_t rtl_freertos_compartment_read(int fd, void *buffer, UBaseType_t offset, size_t count)
{
  if (fd < 0 || fd >= configCOMPARTMENTS_NUM) {
    rtems_rtl_set_error (EBADF, "Invalid compartment/fd number");
    return -1;
  }

  if (offset + count > comp_list[fd].size) {
    count = (offset + count) - comp_list[fd].size;
  }

  if (memcpy(buffer, comp_list[fd].cap + offset, count)) {
    return count;
  }

  return -1;
}

size_t rtl_freertos_compartment_getsize(int fd) {
  return comp_list[fd].size;
}

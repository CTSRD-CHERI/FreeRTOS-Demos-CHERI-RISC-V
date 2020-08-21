#include <stdbool.h>

#include <FreeRTOS.h>
#include "semphr.h"
#include "task.h"

static SemaphoreHandle_t _RTEMS_Allocator_Mutex = NULL;

void _RTEMS_Lock_allocator( void )
{
  if(_RTEMS_Allocator_Mutex == NULL) {
    _RTEMS_Allocator_Mutex = xSemaphoreCreateRecursiveMutex();
    configASSERT(_RTEMS_Allocator_Mutex != NULL);
  }

  xSemaphoreTakeRecursive( _RTEMS_Allocator_Mutex, portMAX_DELAY );
}

void _RTEMS_Unlock_allocator( void )
{
  xSemaphoreGiveRecursive( _RTEMS_Allocator_Mutex );
}

bool _RTEMS_Allocator_is_owner( void )
{
  return xSemaphoreGetMutexHolder(_RTEMS_Allocator_Mutex) == xTaskGetCurrentTaskHandle();
}

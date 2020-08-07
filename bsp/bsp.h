/*****************************************************************************/
/**
*
* @file bsp.h
* @addtogroup bsp
* @{
*
* Test
* ## This is markdown example
*
* **More** markdown
*
* @note
*
* Lorem impsum
*
*
******************************************************************************/
#ifndef RISCV_GENERIC_BSP_H
#define RISCV_GENERIC_BSP_H

#include "stdint.h"
#include "plic_driver.h"

extern plic_instance_t Plic;

void prvSetupHardware(void);
void external_interrupt_handler(uint32_t cause);

/**
 * Exit the simulator with a status code
 */
void vTerminate( int32_t lExitCode );

/**
 * Xilinx Drivers defines
 * Some xillinx drivers require to sleep for given number of seconds
 */
#include "FreeRTOS.h"
#include "task.h"

#define sleep(_SECS) vTaskDelay(pdMS_TO_TICKS(_SECS * 1000));
#define msleep(_MSECS) vTaskDelay(pdMS_TO_TICKS(_MSECS));

#endif /* RISCV_GENERIC_BSP_H */

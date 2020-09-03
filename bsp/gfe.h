#ifndef RISCV_GFE_PLATFORM_H
#define RISCV_GFE_PLATFORM_H

#include "stdint.h"
#include "plic_driver.h"

#define configCLINT_BASE_ADDRESS 0x10000000
#define CLINT_CTRL_ADDR 0x10000000

#ifndef configCPU_CLOCK_HZ
#define configCPU_CLOCK_HZ ((uint32_t)(100000000))
#endif

#ifndef configPERIPH_CLOCK_HZ
#define configPERIPH_CLOCK_HZ ((uint32_t)(100000000))
#endif

/**
 * PLIC defines
 */
#define PLIC_BASE_ADDR (0xC000000ULL)

#define PLIC_NUM_SOURCES 16
#define PLIC_NUM_PRIORITIES 16

#define PLIC_SOURCE_UART0 0x1
#define PLIC_SOURCE_ETH 0x2
#define PLIC_SOURCE_DMA_MM2S 0x3
#define PLIC_SOURCE_DMA_S2MM 0x4

#define PLIC_SOURCE_SPI0 0x5
#define PLIC_SOURCE_UART1 0x6
#define PLIC_SOURCE_IIC0 0x7
#define PLIC_SOURCE_SPI1 0x8

#define PLIC_PRIORITY_UART0 0x1
#define PLIC_PRIORITY_ETH 0x2
#define PLIC_PRIORITY_DMA_MM2S 0x3
#define PLIC_PRIORITY_DMA_S2MM 0x3

#define PLIC_PRIORITY_SPI0 0x3
#define PLIC_PRIORITY_UART1 0x1
#define PLIC_PRIORITY_IIC0 0x3
#define PLIC_PRIORITY_SPI1 0x2

extern plic_instance_t Plic;

#if __riscv_xlen == 64
#define MCAUSE_EXTERNAL_INTERRUPT 0x800000000000000b
#define HANDLER_DATATYPE uint64_t
#else
#define MCAUSE_EXTERNAL_INTERRUPT 0x8000000b
#define HANDLER_DATATYPE uint32_t
#endif

void prvSetupHardware(void);

/**
 * UART defines
 */
#define configUART16550_BASE (0x62300000ULL)
#define configUART16550_BAUD (115200LL)
#define configUART16550_REGSHIFT (2)

#endif /* RISCV_GFE_PLATFORM_H */

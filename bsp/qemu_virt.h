#ifndef RISCV_QEMU_VIRT_PLATFORM_H
#define RISCV_QEMU_VIRT_PLATFORM_H

#define configCLINT_BASE_ADDRESS             0x2000000
#define configUART16550_BASE                 0x10000000
#define configUART16550_BAUD                 115200LL
#define configUART16550_REGSHIFT             1

#define configCPU_CLOCK_HZ                   ( ( unsigned long ) ( 10000000 ) )
#define configPERIPH_CLOCK_HZ                ( ( unsigned long ) ( 10000000 ) )

/**
 * VIRTIO defines
 */
#define QEMU_VIRT_NET_MMIO_ADDRESS           0x10008000
#define QEMU_VIRT_NET_MMIO_SIZE              0x1000
#define QEMU_VIRT_NET_PLIC_INTERRUPT_ID      0x8
#define QEMU_VIRT_NET_PLIC_INTERRUPT_PRIO    0x1

/**
 * PLIC defines
 */
#define PLIC_BASE_ADDR                       ( 0xC000000ULL )

#define PLIC_NUM_SOURCES                     127
#define PLIC_NUM_PRIORITIES                  7

#endif /* RISCV_QEMU_VIRT_PLATFORM_H */

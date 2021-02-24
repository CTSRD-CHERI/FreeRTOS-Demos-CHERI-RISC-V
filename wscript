#-
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Hesham Almatary
#
# This software was developed by SRI International and the University of
# Cambridge Computer Laboratory (Department of Computer Science and
# Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
# DARPA SSITH research programme.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

import os
import subprocess
import ipaddress
from waflib.Task import Task
from waflib.TaskGen import after, before_method, feature
from waflib.TaskGen import extension

top = '.'
out = 'build'


class Toolchain:
    ldflags = []
    cflags = []
    asflags = []

    def cflags_append(self, flags):
        self.cflags.append(flags)

    def asflags_append(self, flags):
        self.aslags.append(flags)

    def ldflags_append(self, flags):
        self.ldflags.append(flags)


class Llvm(Toolchain):
    target = []


class FreeRTOSCompartment:
    # TODO
    pass


class Compartmentalize(Task):
    # TODO
    def run(self):
        return self.exec_command('echo "Compartmentalizing FreeRTOS"')


########################### ARCHs START ###############################
class FreeRTOSArch(Toolchain):
    def srcs_append(self, flags):
        self.srcs.append(flags)


class FreeRTOSArchRiscv(FreeRTOSArch, Toolchain):
    def __init__(self):
        pass

    def add_options(self, ctx):
        ctx.add_option('--riscv-arch',
                       action='store',
                       default="rv64imac",
                       help='RISC-V Architectures')
        ctx.add_option('--riscv-abi',
                       action='store',
                       default="lp64",
                       help='RISC-V ABI')
        ctx.add_option('--purecap',
                       action='store_true',
                       default=False,
                       help='RISC-V CHER Purecap mode')


########################### ARCHs END ################################


########################### BSPS START ###############################
class FreeRTOSBsp(Toolchain):
    name = ""
    platform = ""

    def __init__(self, ctx):
        pass

    def add_options(self, ctx):
        ctx.add_option('--mem-start',
                       action='store',
                       default=0x80000000,
                       help='BSP platform RAM start')


class FreeRTOSBspQemuVirt(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "qemu_virt"

        self.srcs = ['./bsp/uart16550.c', './bsp/sifive_test.c']

    @staticmethod
    def configure(ctx):
        ctx.define('PLATFORM_QEMU_VIRT', 1)
        ctx.define('configCLINT_BASE_ADDRESS', 0x2000000)
        ctx.define('configUART16550_BASE', 0x10000000)
        ctx.define('configUART16550_BAUD', 115200)
        ctx.define('configUART16550_REGSHIFT', 1)
        ctx.define('configCPU_CLOCK_HZ', 10000000)
        ctx.define('configPERIPH_CLOCK_HZ', 10000000)
        ctx.define('configHAS_VIRTIO', 1)
        ctx.define('VIRTIO_USE_MMIO', 1)
        ctx.define('configHAS_VIRTIO_NET', 1)
        ctx.define('VIRTIO_NET_MMIO_ADDRESS', 0x10008000)
        ctx.define('VIRTIO_NET_MMIO_SIZE', 0x1000)
        ctx.define('VIRTIO_NET_PLIC_INTERRUPT_ID', 0x8)
        ctx.define('VIRTIO_NET_PLIC_INTERRUPT_PRIO', 0x1)
        ctx.define('VIRTIO_BLK_MMIO_ADDRESS', 0x10007000)
        ctx.define('VIRTIO_BLK_MMIO_SIZE', 0x1000 )
        ctx.define('PLIC_BASE_ADDR', 0xC000000)
        ctx.define('PLIC_BASE_SIZE', 0x400000)
        ctx.define('PLIC_NUM_SOURCES', 127)
        ctx.define('PLIC_NUM_PRIORITIES', 7)

        if ctx.env.VIRTIO_BLK:
            ctx.define('configHAS_VIRTIO_BLK', 1)

class FreeRTOSBspSpike(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "spike"

        self.srcs = ['./bsp/htif.c']

    @staticmethod
    def configure(ctx):
        ctx.define('PLATFORM_SPIKE', 1)
        ctx.define('configCLINT_BASE_ADDRESS', 0x2000000)


class FreeRTOSBspSail(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "sail"

        self.srcs = ['./bsp/htif.c']

    @staticmethod
    def configure(ctx):
        ctx.define('PLATFORM_SAIL', 1)
        ctx.define('configCLINT_BASE_ADDRESS', 0x2000000)


class FreeRTOSBspPiccolo(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "piccolo"

        self.srcs = ['./bsp/uart16550.c']

    @staticmethod
    def configure(ctx):
        ctx.define('PLATFORM_PICCOLO', 1)
        ctx.define('configCLINT_BASE_ADDRESS', 0x2000000)
        ctx.define('configUART16550_BASE', 0xC0000000)
        ctx.define('configUART16550_BAUD', 115200)
        ctx.define('configUART16550_REGSHIFT', 1)


class FreeRTOSBspGfe(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "gfe"
        self.bld_ctx = ctx,

        self.srcs = ['./bsp/uart16550.c']

        # Xilinx Drivers
        self.srcs += [
            '../RISC-V_Galois_demo/bsp/xilinx/axidma/xaxidma_bd.c',
            '../RISC-V_Galois_demo/bsp/xilinx/axidma/xaxidma_bdring.c',
            '../RISC-V_Galois_demo/bsp/xilinx/axidma/xaxidma.c',
            '../RISC-V_Galois_demo/bsp/xilinx/axidma/xaxidma_selftest.c',
            '../RISC-V_Galois_demo/bsp/xilinx/axidma/xaxidma_g.c',
            '../RISC-V_Galois_demo/bsp/xilinx/axidma/xaxidma_sinit.c',
            '../RISC-V_Galois_demo/bsp/xilinx/axiethernet/xaxiethernet.c',
            '../RISC-V_Galois_demo/bsp/xilinx/axiethernet/xaxiethernet_control.c',
            '../RISC-V_Galois_demo/bsp/xilinx/axiethernet/xaxiethernet_g.c',
            '../RISC-V_Galois_demo/bsp/xilinx/axiethernet/xaxiethernet_sinit.c',
            '../RISC-V_Galois_demo/bsp/xilinx/common/xbasic_types.c',
            '../RISC-V_Galois_demo/bsp/xilinx/common/xil_io.c',
            '../RISC-V_Galois_demo/bsp/xilinx/common/xil_assert.c',
        ]

    @staticmethod
    def configure(ctx):
        ctx.define('PLATFORM_GFE', 1)
        ctx.define('configCLINT_BASE_ADDRESS', 0x10000000)
        ctx.define('CLINT_CTRL_ADDR', 0x10000000)
        ctx.define('configUART16550_BASE', 0x62300000)
        ctx.define('configUART16550_BAUD', 115200)
        ctx.define('configUART16550_REGSHIFT', 2)
        ctx.define('PLIC_BASE_ADDR', 0xC000000)
        ctx.define('PLIC_BASE_SIZE', 0x400000)
        ctx.define('PLIC_NUM_SOURCES', 16)
        ctx.define('PLIC_NUM_PRIORITIES', 16)
        ctx.define('PLIC_SOURCE_UART0', 0x1)
        ctx.define('PLIC_SOURCE_ETH', 0x2)
        ctx.define('PLIC_SOURCE_DMA_MM2S', 0x3)
        ctx.define('PLIC_SOURCE_DMA_S2MM', 0x4)
        ctx.define('PLIC_SOURCE_SPI0', 0x5)
        ctx.define('PLIC_SOURCE_UART1', 0x6)
        ctx.define('PLIC_SOURCE_IIC0', 0x7)
        ctx.define('PLIC_SOURCE_SPI1', 0x8)
        ctx.define('PLIC_PRIORITY_UART0', 0x1)
        ctx.define('PLIC_PRIORITY_ETH', 0x2)
        ctx.define('PLIC_PRIORITY_DMA_MM2S', 0x3)
        ctx.define('PLIC_PRIORITY_DMA_S2MM', 0x3)
        ctx.define('PLIC_PRIORITY_SPI0', 0x3)
        ctx.define('PLIC_PRIORITY_UART1', 0x1)
        ctx.define('PLIC_PRIORITY_IIC0', 0x3)
        ctx.define('PLIC_PRIORITY_SPI1', 0x2)

        if 'p3' in ctx.env.PLATFORM:
            ctx.define('configCPU_CLOCK_HZ', 25000000)
            ctx.define('configPERIPH_CLOCK_HZ', 25000000)
            ctx.define('configMTIME_HZ', 250000)
        else:
            ctx.define('configCPU_CLOCK_HZ', 100000000)
            ctx.define('configPERIPH_CLOCK_HZ', 100000000)

        # Galois/Xilinx defines

        # DMA defines
        ctx.define('BSP_USE_DMA', 1)
        ctx.define('XPAR_XAXIDMA_NUM_INSTANCES', 1)
        ctx.define('XPAR_AXI_DMA', 1)
        ctx.define('XPAR_AXIDMA_0_DEVICE_ID', 0)
        ctx.define('XPAR_AXIDMA_0_BASEADDR', '0x62200000ULL', False, 'Virtual base address of DMA engine')
        ctx.define('XPAR_AXIDMA_0_SIZE', 0x10000)
        ctx.define('XPAR_AXIDMA_0_SG_INCLUDE_STSCNTRL_STRM', 1, 'Control/status stream')
        ctx.define('XPAR_AXIDMA_0_INCLUDE_MM2S', 1, 'AXI4 memory-mapped to stream')
        ctx.define('XPAR_AXIDMA_0_INCLUDE_MM2S_DRE', 1)
        ctx.define('XPAR_AXIDMA_0_M_AXI_MM2S_DATA_WIDTH', 32, 'Allow unaligned transfers')
        ctx.define('XPAR_AXIDMA_0_INCLUDE_S2MM', 1, 'AXI stream to memory-mapped')
        ctx.define('XPAR_AXIDMA_0_INCLUDE_S2MM_DRE', 1)
        ctx.define('XPAR_AXIDMA_0_M_AXI_S2MM_DATA_WIDTH', 32, 'Allow unaligned transfers')
        ctx.define('XPAR_AXIDMA_0_INCLUDE_SG', 1)
        ctx.define('XPAR_AXIDMA_0_NUM_MM2S_CHANNELS', 1)
        ctx.define('XPAR_AXIDMA_0_NUM_S2MM_CHANNELS', 1)
        ctx.define('XPAR_AXI_DMA_0_MM2S_BURST_SIZE', 16)
        ctx.define('XPAR_AXI_DMA_0_S2MM_BURST_SIZE', 16)
        ctx.define('XPAR_AXI_DMA_0_MICRO_DMA', 0)
        ctx.define('XPAR_AXI_DMA_0_ADDR_WIDTH', 64)
        ctx.define('XPAR_AXIDMA_0_SG_LENGTH_WIDTH', 16)

        # Ethernet defines
        ctx.define('BSP_USE_ETHERNET', 1)
        ctx.define('XPAR_XAXIETHERNET_NUM_INSTANCES', 1)
        ctx.define('XPAR_AXIETHERNET_0_PHYADDR', 0x03)
        ctx.define('XPAR_AXIETHERNET_0_DEVICE_ID', 0)
        ctx.define('XPAR_AXIETHERNET_0_BASEADDR', '0x62100000ULL', False)
        ctx.define('XPAR_AXIETHERNET_0_SIZE', 0x40000)
        ctx.define('XPAR_AXIETHERNET_0_TEMAC_TYPE', 2, '0 for SoftTemac at 10/100 Mbps, 1 for SoftTemac at 10/100/1000 Mbps and 2 for Vitex6 Hard Temac')
        ctx.define('XPAR_AXIETHERNET_0_TXCSUM', 0, 'TxCsum indicates that the device has checksum offload on the Tx channel or not.')
        ctx.define('XPAR_AXIETHERNET_0_RXCSUM', 0, 'RxCsum indicates that the device has checksum offload on the Rx channel or not')
        ctx.define('XPAR_AXIETHERNET_0_PHY_TYPE', 'XAE_PHY_TYPE_SGMII', False, 'PhyType indicates which type of PHY interface is used (MII, GMII, RGMII, etc.)')
        ctx.define('XPAR_AXIETHERNET_0_TXVLAN_TRAN', 0)
        ctx.define('XPAR_AXIETHERNET_0_RXVLAN_TRAN', 0)
        ctx.define('XPAR_AXIETHERNET_0_TXVLAN_TAG', 0)
        ctx.define('XPAR_AXIETHERNET_0_RXVLAN_TAG', 0)
        ctx.define('XPAR_AXIETHERNET_0_TXVLAN_STRP', 0)
        ctx.define('XPAR_AXIETHERNET_0_RXVLAN_STRP', 0)
        ctx.define('XPAR_AXIETHERNET_0_MCAST_EXTEND', 0, 'Extended multicast address filtering')
        ctx.define('XPAR_AXIETHERNET_0_STATS', 1, 'Statistics gathering options')
        ctx.define('XPAR_AXIETHERNET_0_AVB', 0, 'Ethernet Audio Video Bridging')
        ctx.define('XPAR_AXIETHERNET_0_ENABLE_SGMII_OVER_LVDS', 1, 'SGMII over LVDS')
        ctx.define('XPAR_AXIETHERNET_0_ENABLE_1588', 0, 'Enable 1588 option')
        ctx.define('XPAR_AXIETHERNET_0_SPEED', 'XAE_SPEED_1000_MBPS', False, 'Tells whether MAC is 1G or 2p5G')
        ctx.define('XPAR_AXIETHERNET_0_NUM_TABLE_ENTRIES', 4, ' Number of table entries for the multicast address filtering')
        ctx.define('XPAR_AXIETHERNET_0_INTR', 'PLIC_SOURCE_ETH', False, 'Axi Ethernet interrupt ID.')
        ctx.define('XPAR_AXIETHERNET_0_CONNECTED_TYPE', 'XPAR_AXI_DMA', False, 'AxiDevType is the type of device attached to the Axi Ethernets AXI4-Stream interface.')
        ctx.define('XPAR_AXIETHERNET_0_CONNECTED_BASEADDR', '0x62200000ULL', False, 'AxiDevBaseAddress is the base address of the device attached to the Axi Ethernets AXI4-Stream interface.')
        ctx.define('XPAR_AXIETHERNET_0_FIFO_INTR', 0xFF)
        ctx.define('XPAR_AXIETHERNET_0_DMA_RX_INTR', 'PLIC_SOURCE_DMA_S2MM', False)
        ctx.define('XPAR_AXIETHERNET_0_DMA_TX_INTR', 'PLIC_SOURCE_DMA_MM2S', False)

        # GPIO defines
        ctx.define('BSP_USE_GPIO', 1)
        ctx.define('XPAR_XGPIO_NUM_INSTANCES', 1)
        ctx.define('XPAR_GPIO_0_DEVICE_ID', 0)
        ctx.define('XPAR_GPIO_0_BASEADDR', '0x62330000ULL', False)
        ctx.define('XPAR_GPIO_0_SIZE', 0x1000)
        ctx.define('XPAR_GPIO_0_INTERRUPT_PRESENT', 0)
        ctx.define('XPAR_GPIO_0_IS_DUAL', 0)

        # IIC Defines
        ctx.define('BSP_USE_IIC0', 1)
        ctx.define('XPAR_XIIC_NUM_INSTANCES', 1)
        ctx.define('XPAR_IIC_0_DEVICE_ID', 0)
        ctx.define('XPAR_IIC_0_BASEADDR', '0x62310000ULL', False)
        ctx.define('XPAR_IIC_0_SIZE', 0x1000)
        ctx.define('XPAR_IIC_0_TEN_BIT_ADR', 0)
        ctx.define('XPAR_IIC_0_GPO_WIDTH', 32)

        # SPI defines
        ctx.define('XPAR_XSPI_NUM_INSTANCES', 2)
        ctx.define('XPAR_SPI_USE_POLLING_MODE', 1)
        ctx.define('BSP_USE_SPI0', 0, False, "SPI0 is Flash memory, don't use it directly")
        ctx.define('XPAR_SPI_0_DEVICE_ID', 0)
        ctx.define('XPAR_SPI_0_BASEADDR', '0x4000000ULL', False)
        ctx.define('XPAR_SPI_0_SIZE', 0x1000)
        ctx.define('XPAR_SPI_0_FIFO_EXIST', 0)
        ctx.define('XPAR_SPI_0_SLAVE_ONLY', 0)
        ctx.define('XPAR_SPI_0_NUM_SS_BITS', 0)
        ctx.define('XPAR_SPI_0_NUM_TRANSFER_BITS', 0)
        ctx.define('XPAR_SPI_0_SPI_MODE', 0)
        ctx.define('XPAR_SPI_0_AXI_INTERFACE', 0)
        ctx.define('XPAR_SPI_0_AXI_FULL_BASEADDR', 0)
        ctx.define('XPAR_SPI_0_XIP_MODE', 0)
        ctx.define('XPAR_SPI_0_USE_STARTUP', 0)

        ctx.define('BSP_USE_SPI1', 0, False, "SPI1 is used for SD card (polled mode), and can be used for LCD screen (interrupt mode)")
        ctx.define('XPAR_SPI_1_DEVICE_ID', 1)
        ctx.define('XPAR_SPI_1_BASEADDR', '0x6232000ULL', False)
        ctx.define('XPAR_SPI_1_SIZE', 0x1000)
        ctx.define('XPAR_SPI_1_FIFO_EXIST', 0)
        ctx.define('XPAR_SPI_1_SLAVE_ONLY', 0)
        ctx.define('XPAR_SPI_1_NUM_SS_BITS', 1)
        ctx.define('XPAR_SPI_1_NUM_TRANSFER_BITS', 8)
        ctx.define('XPAR_SPI_1_SPI_MODE', 0)
        ctx.define('XPAR_SPI_1_AXI_INTERFACE', 0)
        ctx.define('XPAR_SPI_1_AXI_FULL_BASEADDR', 0)
        ctx.define('XPAR_SPI_1_XIP_MODE', 0)
        ctx.define('XPAR_SPI_1_USE_STARTUP', 0)

        if ctx.env.RISCV_XLEN == '64':
            ctx.define('MCAUSE_EXTERNAL_INTERRUPT', '0x800000000000000b', False)
        else:
            ctx.define('MCAUSE_EXTERNAL_INTERRUPT', '0x8000000b', False)

        ctx.env.MEMSTART = 0xC0000000

        ctx.env.append_value('INCLUDES', [
            ctx.path.abspath() + '/../RISC-V_Galois_demo/bsp/xilinx',
            ctx.path.abspath() + '/../RISC-V_Galois_demo/bsp/xilinx/common',
            ctx.path.abspath() + '/../RISC-V_Galois_demo/bsp/xilinx/axidma',
            ctx.path.abspath() + '/../RISC-V_Galois_demo/bsp/xilinx/axiethernet',
        ])

class FreeRTOSBspFett(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "fett"
        self.bld_ctx = ctx,

        self.srcs = ['./bsp/uart16550.c', './bsp/sifive_test.c']

    @staticmethod
    def configure(ctx):
        ctx.define('PLATFORM_FETT', 1)
        ctx.define('configCLINT_BASE_ADDRESS', 0x10000000)
        ctx.define('CLINT_CTRL_ADDR', 0x10000000)
        ctx.define('configUART16550_BASE', 0x62300000)
        ctx.define('configUART16550_BAUD', 115200)
        ctx.define('configUART16550_REGSHIFT', 2)
        ctx.define('SIFIVE_TEST_BASE', 0x50000000)
        ctx.define('configCPU_CLOCK_HZ', 100000000)
        ctx.define('configPERIPH_CLOCK_HZ', 250000000)
        ctx.define('PLIC_BASE_ADDR', 0xC000000)
        ctx.define('PLIC_BASE_SIZE', 0x400000)
        ctx.define('PLIC_NUM_SOURCES', 16)
        ctx.define('PLIC_NUM_PRIORITIES', 7)
        ctx.define('PLIC_SOURCE_UART0', 0x1)
        ctx.define('PLIC_PRIORITY_UART0', 0x1)
        ctx.define('configHAS_VIRTIO', 1)
        ctx.define('VIRTIO_USE_MMIO', 1)
        ctx.define('configHAS_VIRTIO_NET', 1)
        ctx.define('VIRTIO_NET_MMIO_ADDRESS', 0x40000000)
        ctx.define('VIRTIO_NET_MMIO_SIZE', 0x1000)
        ctx.define('VIRTIO_BLK_MMIO_ADDRESS', 0x40002000)
        ctx.define('VIRTIO_BLK_MMIO_SIZE', 0x1000)
        ctx.define('VIRTIO_NET_PLIC_INTERRUPT_ID', 0x2)
        ctx.define('VIRTIO_NET_PLIC_INTERRUPT_PRIO', 0x1)

        if ctx.env.RISCV_XLEN == '64':
            ctx.define('MCAUSE_EXTERNAL_INTERRUPT', 0x800000000000000b)
        else:
            ctx.define('MCAUSE_EXTERNAL_INTERRUPT', 0x8000000b)

        if ctx.env.VIRTIO_BLK:
            ctx.define('configHAS_VIRTIO_BLK', 1)

        ctx.env.MEMSTART = 0xC0000000


########################### BSPS END ###############################


########################### LIBS START #############################
class FreeRTOSLib:
    name = ""
    srcs = []
    includes = []
    defines = []
    export_includes = []

    def __init__(self, bld):
        bld.env.append_value('INCLUDES', self.export_includes)
        bld.env.append_value('DEFINES', self.defines)
        self.is_compartment = False
        self.cflags = []

    def build(self, bld):
        bld(export_includes=self.export_includes, name=self.name + "_headers")

        if self.is_compartment:
            self.cflags += ['-cheri-cap-table-abi=gprel']

        bld.stlib(features=['c'],
                  asflags=bld.env.CFLAGS + bld.env.ASFLAGS,
                  cflags = bld.env.CFLAGS + self.cflags,
                  includes=bld.env.INCLUDES + self.includes,
                  export_includes=self.export_includes,
                  source=self.srcs,
                  target=self.name),


class FreeRTOSLibCore(FreeRTOSLib):
    freertos_core_dir = '../../Source/'

    def __init__(self, ctx):
        self.name = "freertos_core"

        ctx.env.append_value('ASFLAGS', [
            '-Wa,-Ilegacy',
            '-DportasmHANDLE_INTERRUPT=external_interrupt_handler'
        ])

        self.includes = [
            self.freertos_core_dir + '/include',
            self.freertos_core_dir + '/portable/GCC/RISC-V',
            self.freertos_core_dir +
            'portable/GCC/RISC-V/chip_specific_extensions/RV32I_CLINT_no_extensions'
        ]

        self.export_includes = [
            self.freertos_core_dir + '/include',
            self.freertos_core_dir + '/portable/GCC/RISC-V',
            self.freertos_core_dir +
            'portable/GCC/RISC-V/chip_specific_extensions/RV32I_CLINT_no_extensions'
        ]

        self.srcs = [
            self.freertos_core_dir + 'croutine.c', self.freertos_core_dir +
            'list.c', self.freertos_core_dir + 'queue.c',
            self.freertos_core_dir + 'tasks.c', self.freertos_core_dir +
            'timers.c', self.freertos_core_dir + 'event_groups.c',
            self.freertos_core_dir + 'stream_buffer.c',
            self.freertos_core_dir + 'portable/MemMang/heap_4.c',
            self.freertos_core_dir + 'portable/GCC/RISC-V/port.c',
            self.freertos_core_dir +
            'portable/GCC/RISC-V/chip_specific_extensions/CHERI/portASM.S'
            if ctx.env.PURECAP else self.freertos_core_dir +
            'portable/GCC/RISC-V/portASM.S'
        ]

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibBsp(FreeRTOSLib):
    platform = ""
    arch = ""
    freertos_bsp_dir = "./bsp/"

    def __init__(self, ctx):
        FreeRTOSLib.name = "freertos_bsp"
        self.name = "freertos_bsp"

        self.platform = ctx.env.PLATFORM
        self.arch = ctx.env.ARCH

        self.freertos_platform = ctx.env.freertos_demos[ctx.env.DEMO][
            ctx.env.ARCH][ctx.env.PLATFORM.split('-')[0]]

        #self.defines = self.freertos_platform.defines

        self.includes = [".", self.freertos_bsp_dir]

        self.export_includes = [".", self.freertos_bsp_dir]

        self.srcs = [
            self.freertos_bsp_dir + 'boot.S', self.freertos_bsp_dir + 'bsp.c',
            self.freertos_bsp_dir + 'rand.c', self.freertos_bsp_dir +
            'plic_driver.c', self.freertos_bsp_dir + 'syscalls.c'
        ] + self.freertos_platform.srcs

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibCheri(FreeRTOSLib):

    libcheri_dir = '../../../FreeRTOS-Labs/FreeRTOS-Labs/Source/FreeRTOS-libcheri'

    def __init__(self, ctx):
        self.name = "cheri"
        self.srcs = [self.libcheri_dir + '/cheri/cheri-riscv.c']
        self.includes = [self.libcheri_dir + '/include']
        self.export_includes = [self.libcheri_dir + '/include']

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibDL(FreeRTOSLib):

    libdl_dir = '../../../FreeRTOS-Labs/FreeRTOS-Labs/Source/FreeRTOS-libdl/'

    def __init__(self, ctx):
        self.name = "freertos_libdl"
        self.srcs = [
           self.libdl_dir + 'libdl/dlfcn.c',
           self.libdl_dir + 'libdl/fastlz.c',
           self.libdl_dir + 'libdl/rtl-alloc-heap.c',
           self.libdl_dir + 'libdl/rtl-allocator.c',
           self.libdl_dir + 'libdl/rtl-alloc-lock.c',
           self.libdl_dir + 'libdl/rtl-archive.c',
           self.libdl_dir + 'libdl/rtl-bit-alloc.c',
           self.libdl_dir + 'libdl/rtl-chain-iterator.c',
           self.libdl_dir + 'libdl/rtl-debugger.c',
           self.libdl_dir + 'libdl/rtl-elf.c',
           self.libdl_dir + 'libdl/rtl-error.c',
           self.libdl_dir + 'libdl/rtl-find-file.c',
           self.libdl_dir + 'libdl/rtl-mdreloc-riscv.c',
           self.libdl_dir + 'libdl/rtl-obj-cache.c',
           self.libdl_dir + 'libdl/rtl-obj-comp.c',
           self.libdl_dir + 'libdl/rtl-obj.c',
           self.libdl_dir + 'libdl/rtl-string.c',
           self.libdl_dir + 'libdl/rtl-sym.c',
           self.libdl_dir + 'libdl/rtl-trace.c',
           self.libdl_dir + 'libdl/rtl-unresolved.c',
           self.libdl_dir + 'libdl/rtl-unwind-dw2.c',
           self.libdl_dir + 'libdl/rtl-freertos-compartments.c',
           self.libdl_dir + 'libdl/rtl.c'
        ]

        self.export_includes = [self.libdl_dir + '/include']

        FreeRTOSLib.__init__(self, ctx)

class FreeRTOSLibVirtIO(FreeRTOSLib):

    libvirtio_dir = '../../../FreeRTOS-Labs/FreeRTOS-Labs/Source/FreeRTOS-libvirtio/'

    def __init__(self, ctx):
        self.name = "virtio"

        self.srcs = [
            self.libvirtio_dir + 'virtio.c',
            self.libvirtio_dir + 'virtio-net.c',
            self.libvirtio_dir + 'helpers.c'
        ]

        if ctx.env.VIRTIO_BLK:
            self.srcs += [self.libvirtio_dir + 'virtio-blk.c']

        self.includes = [self.libvirtio_dir]
        self.export_includes = [self.libvirtio_dir]

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibTCPIP(FreeRTOSLib):

    libtcpip_dir = '../../../FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP'

    def __init__(self, ctx):
        self.name = "freertos_tcpip"
        self.driver_srcs = []

        self.includes = [
            self.libtcpip_dir, self.libtcpip_dir + '/include',
            self.libtcpip_dir + '/portable/Compiler/GCC'
        ]

        self.export_includes = [
            self.libtcpip_dir, self.libtcpip_dir + '/include',
            self.libtcpip_dir + '/portable/Compiler/GCC'
        ]

        if ctx.env.PLATFORM in ["qemu_virt", "fett"]:
            self.driver_srcs = [
                self.libtcpip_dir +
                '/portable/NetworkInterface/virtio/NetworkInterface.c']
        elif 'gfe' in ctx.env.PLATFORM:
            self.driver_srcs = [
                self.libtcpip_dir +
                '/portable/NetworkInterface/RISC-V/riscv_hal_eth.c',
                self.libtcpip_dir +
                '/portable/NetworkInterface/RISC-V/NetworkInterface.c',
            ]

        self.srcs = self.driver_srcs + [
            self.libtcpip_dir + '/FreeRTOS_IP.c', self.libtcpip_dir +
            '/FreeRTOS_ARP.c', self.libtcpip_dir + '/FreeRTOS_DHCP.c',
            self.libtcpip_dir + '/FreeRTOS_DNS.c', self.libtcpip_dir +
            '/FreeRTOS_Sockets.c', self.libtcpip_dir + '/FreeRTOS_TCP_IP.c',
            self.libtcpip_dir + '/FreeRTOS_UDP_IP.c',
            self.libtcpip_dir + '/FreeRTOS_TCP_WIN.c', self.libtcpip_dir +
            '/FreeRTOS_Stream_Buffer.c', self.libtcpip_dir +
            '/portable/BufferManagement/BufferAllocation_2.c'
        ]

        FreeRTOSLib.__init__(self, ctx)

    def build_objects(self, ctx):
        ctx.objects(source=self.srcs,
                    includes=self.export_includes + ctx.env.INCLUDES,
                    export_includes=self.includes,
                    target='freertos_tcpip_objects')

    def build_lib(self, ctx):
        ctx.stlib(source=self.srcs,
                  use=["freertos_tcpip_objects"],
                  target='freertos_tcpip_lib')


class FreeRTOSLibFAT(FreeRTOSLib):

    libfat_dir = '../../../FreeRTOS-Labs/FreeRTOS-Labs/Source/FreeRTOS-Plus-FAT/'

    def __init__(self, ctx):
        self.name = "freertos_fat"

        if ctx.env.VIRTIO_BLK:
            self.srcs = [
                self.libfat_dir + 'portable/virtio-blk/ff_virtioblk_disk.c'
            ]
            self.includes = [self.libfat_dir + 'portable/virtio-blk']
            self.export_includes = [self.libfat_dir + 'portable/virtio-blk']
        else:
            self.srcs = [self.libfat_dir + 'portable/common/ff_ramdisk.c']

        self.srcs += [
            self.libfat_dir + 'ff_crc.c', self.libfat_dir + 'ff_dir.c',
            self.libfat_dir + 'ff_error.c', self.libfat_dir + 'ff_fat.c',
            self.libfat_dir + 'ff_file.c', self.libfat_dir + 'ff_format.c',
            self.libfat_dir + 'ff_ioman.c', self.libfat_dir + 'ff_locking.c',
            self.libfat_dir + 'ff_memory.c', self.libfat_dir + 'ff_string.c',
            self.libfat_dir + 'ff_sys.c', self.libfat_dir + 'ff_time.c',
            self.libfat_dir + 'ff_stdio.c'
        ]

        self.includes += [
            self.libfat_dir + '/include', self.libfat_dir + 'portable/common'
        ]

        self.export_includes += [
            self.libfat_dir + '/include', self.libfat_dir + 'portable/common'
        ]

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibCLI(FreeRTOSLib):

    libcli_dir = '../../../FreeRTOS-Plus/Source/FreeRTOS-Plus-CLI/'

    def __init__(self, ctx):
        self.name = "freertos_cli"
        self.srcs = [
            self.libcli_dir + 'FreeRTOS_CLI.c',
        ]

        self.includes = [self.libcli_dir]

        self.export_includes = [self.libcli_dir]

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibAWSOTA(FreeRTOSLib):

    libawsota_dir = '../../../FreeRTOS-Plus/Source/aws-ota/source/'

    def __init__(self, ctx):
        self.name = "freertos_libota"
        self.srcs = [
            self.libawsota_dir + 'ota.c',
            self.libawsota_dir + 'ota_base64.c',
            self.libawsota_dir + 'ota_cbor.c',
            self.libawsota_dir + 'ota_http.c',
            self.libawsota_dir + 'ota_interface.c',
            self.libawsota_dir + 'ota_mqtt.c',
            self.libawsota_dir + 'portable/os/ota_os_freertos.c',
        ]

        self.includes = [
            self.libawsota_dir + 'include', self.libawsota_dir + 'portable/os/'
        ]

        self.export_includes = [
            self.libawsota_dir + 'include', self.libawsota_dir + 'portable/os/'
        ]

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibcoreJSON(FreeRTOSLib):

    libcorejson_dir = '../../../FreeRTOS-Plus/Source/coreJSON/source/'

    def __init__(self, ctx):
        self.name = "freertos_libcorejson"
        self.srcs = [
            self.libcorejson_dir + 'core_json.c',
        ]

        self.includes = [self.libcorejson_dir + 'include']

        self.export_includes = [self.libcorejson_dir + 'include']

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibcoreMQTT(FreeRTOSLib):

    libcoremqtt_dir = '../../../FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/'

    def __init__(self, ctx):
        self.name = "freertos_libcoremqtt"
        self.srcs = [
            self.libcoremqtt_dir + 'core_mqtt.c',
            self.libcoremqtt_dir + 'core_mqtt_serializer.c',
            self.libcoremqtt_dir + 'core_mqtt_state.c',
        ]

        self.includes = [
            self.libcoremqtt_dir + 'include',
            self.libcoremqtt_dir + 'interface'
        ]

        self.export_includes = [
            self.libcoremqtt_dir + 'include',
            self.libcoremqtt_dir + 'interface'
        ]

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibTinyCbor(FreeRTOSLib):

    libtinycbor_dir = '../../../FreeRTOS-Plus/ThirdParty/tinycbor/'

    def __init__(self, ctx):
        self.name = "libtinycbor"
        self.srcs = [
            self.libtinycbor_dir + 'src/cborencoder.c',
            self.libtinycbor_dir + 'src/cborencoder_close_container_checked.c',
            self.libtinycbor_dir + 'src/cborerrorstrings.c',
            self.libtinycbor_dir + 'src/cborparser.c',
            self.libtinycbor_dir + 'src/cborparser_dup_string.c',
            self.libtinycbor_dir + 'src/cborpretty.c',
            self.libtinycbor_dir + 'src/cborpretty_stdio.c',
            self.libtinycbor_dir + 'src/cborvalidation.c',
        ]

        self.includes = [
            self.libtinycbor_dir + 'src/',
        ]

        self.export_includes = [
            self.libtinycbor_dir + 'src/',
        ]

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibNetworkTransport(FreeRTOSLib):

    libnetworktransport_dir = '../../../FreeRTOS-Plus/Source/Application-Protocols/network_transport/'

    def __init__(self, ctx):
        self.name = "freertos_libnetwork_transport"
        self.srcs = [
            self.libnetworktransport_dir +
            'freertos_plus_tcp/using_mbedtls/using_mbedtls.c',
            self.libnetworktransport_dir +
            'freertos_plus_tcp/using_plaintext/using_plaintext.c',
            self.libnetworktransport_dir +
            'freertos_plus_tcp/sockets_wrapper.c',
        ]

        self.includes = [
            self.libnetworktransport_dir + 'freertos_plus_tcp/using_mbedtls/',
            self.libnetworktransport_dir +
            'freertos_plus_tcp/using_plaintext/',
            self.libnetworktransport_dir + 'freertos_plus_tcp/',
        ]

        self.export_includes = [
            self.libnetworktransport_dir + 'freertos_plus_tcp/using_mbedtls/',
            self.libnetworktransport_dir +
            'freertos_plus_tcp/using_plaintext/',
            self.libnetworktransport_dir + 'freertos_plus_tcp/',
        ]

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibMbedTLS(FreeRTOSLib):

    libmbedtls_dir = '../../../FreeRTOS-Plus/Source/Utilities/mbedtls_freertos/'
    mbedtls_dir = '../../.././FreeRTOS-Plus/ThirdParty/mbedtls/'

    def __init__(self, ctx):
        self.name = "freertos_libmbedtls"
        self.srcs = [
            self.libmbedtls_dir + 'mbedtls_error.c',
            self.libmbedtls_dir + 'mbedtls_freertos_port.c',
            self.mbedtls_dir + 'library/platform_util.c',
            self.mbedtls_dir + 'library/aes.c',
            self.mbedtls_dir + 'library/aesni.c',
            self.mbedtls_dir + 'library/arc4.c',
            self.mbedtls_dir + 'library/asn1parse.c',
            self.mbedtls_dir + 'library/asn1write.c',
            self.mbedtls_dir + 'library/base64.c',
            self.mbedtls_dir + 'library/bignum.c',
            self.mbedtls_dir + 'library/blowfish.c',
            self.mbedtls_dir + 'library/camellia.c',
            self.mbedtls_dir + 'library/ccm.c',
            self.mbedtls_dir + 'library/certs.c',
            self.mbedtls_dir + 'library/cipher.c',
            self.mbedtls_dir + 'library/cipher_wrap.c',
            self.mbedtls_dir + 'library/cmac.c',
            self.mbedtls_dir + 'library/ctr_drbg.c',
            self.mbedtls_dir + 'library/debug.c',
            self.mbedtls_dir + 'library/des.c',
            self.mbedtls_dir + 'library/dhm.c',
            self.mbedtls_dir + 'library/ecdh.c',
            self.mbedtls_dir + 'library/ecdsa.c',
            self.mbedtls_dir + 'library/ecjpake.c',
            self.mbedtls_dir + 'library/ecp.c',
            self.mbedtls_dir + 'library/ecp_curves.c',
            self.mbedtls_dir + 'library/entropy.c',
            self.mbedtls_dir + 'library/entropy_poll.c',
            self.mbedtls_dir + 'library/error.c',
            self.mbedtls_dir + 'library/gcm.c',
            self.mbedtls_dir + 'library/havege.c',
            self.mbedtls_dir + 'library/hmac_drbg.c',
            self.mbedtls_dir + 'library/md.c',
            self.mbedtls_dir + 'library/md2.c',
            self.mbedtls_dir + 'library/md4.c',
            self.mbedtls_dir + 'library/md5.c',
            self.mbedtls_dir + 'library/memory_buffer_alloc.c',
            self.mbedtls_dir + 'library/net_sockets.c',
            self.mbedtls_dir + 'library/oid.c',
            self.mbedtls_dir + 'library/padlock.c',
            self.mbedtls_dir + 'library/pem.c',
            self.mbedtls_dir + 'library/pk.c',
            self.mbedtls_dir + 'library/pk_wrap.c',
            self.mbedtls_dir + 'library/pkcs11.c',
            self.mbedtls_dir + 'library/pkcs12.c',
            self.mbedtls_dir + 'library/pkcs5.c',
            self.mbedtls_dir + 'library/pkparse.c',
            self.mbedtls_dir + 'library/pkwrite.c',
            self.mbedtls_dir + 'library/platform.c',
            self.mbedtls_dir + 'library/ripemd160.c',
            self.mbedtls_dir + 'library/rsa.c',
            self.mbedtls_dir + 'library/rsa_internal.c',
            self.mbedtls_dir + 'library/sha1.c',
            self.mbedtls_dir + 'library/sha256.c',
            self.mbedtls_dir + 'library/sha512.c',
            self.mbedtls_dir + 'library/ssl_cache.c',
            self.mbedtls_dir + 'library/ssl_ciphersuites.c',
            self.mbedtls_dir + 'library/ssl_cli.c',
            self.mbedtls_dir + 'library/ssl_cookie.c',
            self.mbedtls_dir + 'library/ssl_srv.c',
            self.mbedtls_dir + 'library/ssl_ticket.c',
            self.mbedtls_dir + 'library/ssl_tls.c',
            self.mbedtls_dir + 'library/ssl_msg.c',
            self.mbedtls_dir + 'library/threading.c',
            self.mbedtls_dir + 'library/timing.c',
            self.mbedtls_dir + 'library/version.c',
            self.mbedtls_dir + 'library/version_features.c',
            self.mbedtls_dir + 'library/x509.c',
            self.mbedtls_dir + 'library/x509_create.c',
            self.mbedtls_dir + 'library/x509_crl.c',
            self.mbedtls_dir + 'library/x509_crt.c',
            self.mbedtls_dir + 'library/x509_csr.c',
            self.mbedtls_dir + 'library/x509write_crt.c',
            self.mbedtls_dir + 'library/x509write_csr.c',
            self.mbedtls_dir + 'library/xtea.c',
        ]

        self.includes = [
            self.libmbedtls_dir,
            '../../../FreeRTOS-Plus/ThirdParty/mbedtls/include/'
        ]

        self.export_includes = [
            self.libmbedtls_dir,
            '../../../FreeRTOS-Plus/ThirdParty/mbedtls/include/'
        ]

        FreeRTOSLib.__init__(self, ctx)


########################### LIBS END #############################


########################### INIT START #############################
def freertos_libs_init(bld_ctx):
    bld_ctx.env.libs = {}

    bld_ctx.env.libs["freertos_core"] = FreeRTOSLibCore(bld_ctx)
    bld_ctx.env.libs["freertos_bsp"] = FreeRTOSLibBsp(bld_ctx)
    bld_ctx.env.libs["freertos_tcpip"] = FreeRTOSLibTCPIP(bld_ctx)
    bld_ctx.env.libs["freertos_libdl"] = FreeRTOSLibDL(bld_ctx)
    bld_ctx.env.libs["freertos_cli"] = FreeRTOSLibCLI(bld_ctx)
    bld_ctx.env.libs["freertos_fat"] = FreeRTOSLibFAT(bld_ctx)
    bld_ctx.env.libs["cheri"] = FreeRTOSLibCheri(bld_ctx)
    bld_ctx.env.libs["virtio"] = FreeRTOSLibVirtIO(bld_ctx)
    bld_ctx.env.libs["freertos_libota"] = FreeRTOSLibAWSOTA(bld_ctx)
    bld_ctx.env.libs["libtinycbor"] = FreeRTOSLibTinyCbor(bld_ctx)
    bld_ctx.env.libs["freertos_libcorejson"] = FreeRTOSLibcoreJSON(bld_ctx)
    bld_ctx.env.libs["freertos_libcoremqtt"] = FreeRTOSLibcoreMQTT(bld_ctx)
    bld_ctx.env.libs["freertos_libmbedtls"] = FreeRTOSLibMbedTLS(bld_ctx)
    bld_ctx.env.libs[
        "freertos_libnetwork_transport"] = FreeRTOSLibNetworkTransport(bld_ctx)

def freertos_bsp_configure(conf):
    platform = conf.options.riscv_platform

    if platform == 'qemu_virt':
        FreeRTOSBspQemuVirt.configure(conf)
    elif platform == 'spike':
        FreeRTOSBspSpike.configure(conf)
    elif platform == 'sail':
        FreeRTOSBspSail.configure(conf)
    elif 'gfe' in platform:
        FreeRTOSBspGfe.configure(conf)
    elif platform == 'fett':
        FreeRTOSBspFett.configure(conf)
    elif platform == 'piccolo':
        FreeRTOSBspPiccolo.configure(conf)
    else:
        conf.fatal('Failed invalid to configure BSP')

def freertos_bsps_init(bld_ctx):
    bld_ctx.env.freertos_bsps = {}

    bld_ctx.env.freertos_bsps["spike"] = FreeRTOSBspSpike(bld_ctx)
    bld_ctx.env.freertos_bsps["qemu_virt"] = FreeRTOSBspQemuVirt(bld_ctx)
    bld_ctx.env.freertos_bsps["sail"] = FreeRTOSBspSail(bld_ctx)
    bld_ctx.env.freertos_bsps["gfe"] = FreeRTOSBspGfe(bld_ctx)
    bld_ctx.env.freertos_bsps["fett"] = FreeRTOSBspFett(bld_ctx)
    bld_ctx.env.freertos_bsps["piccolo"] = FreeRTOSBspPiccolo(bld_ctx)


def freertos_demos_init(bld):
    bld.env.freertos_demos = {}
    bld.env.freertos_demos["RISC-V-Generic"] = {}
    bld.env.freertos_demos["RISC-V-Generic"]["riscv64"] = bld.env.freertos_bsps


def freertos_init(bld):
    freertos_bsps_init(bld)
    freertos_demos_init(bld)
    freertos_libs_init(bld)


########################### INIT END #############################

########################### UTILS START #############################

# Size in MiB
def create_disk_image(ctx, size=5):

    with open(ctx.env.PREFIX + '/bin/freertos.img', 'wb') as f:
        filepath = ctx.env.PREFIX + '/bin/freertos.img'
        mtoolsrcpath = ctx.env.PREFIX + '/mtoolsrc'
        libdl_config_path = ctx.env.PREFIX + '/libdl.conf'
        os.environ["MTOOLSRC"] = mtoolsrcpath
        f.seek(1024 * 1024 * size)
        f.write('0')
        f.flush()

        # If the user hasn't asked to partition/format/write a disk image, skip
        if not ctx.env.CREATE_DISK_IMAGE:
            return

        # Imply disk geometry
        sectors = 8
        tracks = 256
        heads = (size * 1024 * 1024) / (512 * sectors * tracks)

        # Create an mtools conf file read by the following commands
        with open(mtoolsrcpath, 'wb') as mtools_conf:
            mtools_conf.write('drive /: file="' + filepath + '" partition=1')

        # Initialize the parition table to FAT32 and remove any paritions
        subprocess.Popen(["mpartition", '-I', '-s', str(sectors), '-t', str(tracks) ,'-h', str(heads), '-T', '0x0C', "/:"])

        # Create a single primary FAT32 parition occupying the entire disk
        subprocess.Popen(["mpartition", '-cpva', '-s', str(sectors), '-t', str(tracks) ,'-h', str(heads), '-T', '0x0C', "/:"])

        # Format the disk/parition with 8 hidden sectors
        subprocess.Popen(["mformat", '-H', '8', "/:"])

        # Create /lib/ directory
        subprocess.Popen(["mmd", "lib"])

        # Create /etc/ directory
        subprocess.Popen(["mmd", "etc"])

        # Get the libraries to write in the disk image
        LIBS_TO_EMBED = ["lib" + lib + ".a" for lib in ctx.env.LIB_DEPS_EMBED_FAT]

        # Create a libdl.conf file that contains the list of libraries, read by libdl
        with open(libdl_config_path, 'wb') as libdlconf:
            libdl_config = '\n'.join(['/lib/' + lib for lib in LIBS_TO_EMBED])
            libdl_config += '\n'
            libdlconf.write(libdl_config)
            subprocess.Popen(["mcd", "/etc/"])
            subprocess.Popen(["mcopy", '-tvpn', libdl_config_path, '/:'+'libdl.conf'])

        # Write the libraries to /lib/
        for lib in LIBS_TO_EMBED:
            lib_path = str(ctx.path.get_bld().ant_glob('**/' + lib, quiet=True)[0])
            subprocess.Popen(["mcd", "/lib/"])
            subprocess.Popen(["mcopy", '-vpn', lib_path, '/:/lib/'+lib],)

########################### UTILS END   #############################


def options(ctx):
    # Tools
    ctx.load('compiler_c')

    arch_riscv = FreeRTOSArchRiscv()
    arch_riscv.add_options(ctx)

    # Demo options
    ctx.add_option('--prefix',
                   action='store',
                   default="/usr/local",
                   help='The FreeRTOS prefix for install.')
    ctx.add_option('--demo',
                   action='store',
                   default="RISC-V-Generic",
                   help='The FreeRTOS Demo build.')
    ctx.add_option('--program',
                   action='store',
                   default="main_servers",
                   help='The FreeRTOS program to build.')
    ctx.add_option('--program-entry',
                   action='store',
                   default="main",
                   help='The FreeRTOS program to build.')
    ctx.add_option('--toolchain',
                   action='store',
                   default="llvm",
                   help='The toolchain to build FreeRTOS with')
    ctx.add_option('--sysroot',
                   action='store',
                   default="/opt",
                   help='The sysroot')

    ctx.add_option(
        '--program-path',
        action='store',
        default=None,
        help='The path to FreeRTOS program to build, containing a wscript.')

    bsp = FreeRTOSBsp(ctx)

    ctx.add_option('--debug',
                   action='store_true',
                   default=False,
                   help='Enable debug build')

    # BSP options
    ctx.add_option('--riscv-platform',
                   action='store',
                   default="qemu_virt",
                   help='RISC-V Platform/Board')
    ctx.add_option('--mem-start',
                   action='store',
                   default=0x80000000,
                   help='BSP platform RAM start')

    ctx.add_option('--use-virtio-blk',
                   action='store_true',
                   default=False,
                   help='Use VirtIO Block Device for the file system')

    ctx.add_option('--create-disk-image',
                   action='store_true',
                   default=False,
                   help='Create and write data into an external disk image')

    # Features options
    ctx.add_option('--compartmentalize',
                   action='store_true',
                   default=False,
                   help='Expermintal CHERI-based SW compartmentalization')

    ctx.add_option('--compartmentalization_mode',
                   action='store',
                   default="objs",
                   help='Comparmentalization mode (either objs or libs)')

    # IP options
    ctx.add_option('--ipaddr',
                   action='store',
                   default='10.0.2.15/24',
                   help='Static IP Address for FreeRTOS')

    ctx.add_option('--gateway',
                   action='store',
                   default='10.0.2.2',
                   help='Gateway address for FreeRTOS')

    # Run options
    ctx.add_option('--run',
                   action='store_true',
                   default=False,
                   help='Run the program after it is built')

def ipaddr_freertos_ipconfig(ip, gateway, ctx):
    try:
        interface = ipaddress.ip_interface(ip)
        gateway = ipaddress.ip_interface(gateway)
        ipaddr_str, netmask_str = interface.with_netmask.split('/')
        gateway_str = str(gateway.ip)
        print('FreeRTOS IP Configs -> ', ipaddr_str, '/', netmask_str)

        ipaddr_arr = ipaddr_str.split('.')
        netmask_arr = netmask_str.split('.')
        gateway_arr = gateway_str.split('.')
        for index in range(len(ipaddr_arr)):
            ctx.define('configIP_ADDR'+str(index), int(ipaddr_arr[index]))
        for index in range(len(netmask_arr)):
            ctx.define('configNET_MASK'+str(index), int(netmask_arr[index]))
        for index in range(len(gateway_arr)):
            ctx.define('configGATEWAY_ADDR'+str(index), int(gateway_arr[index]))
    except:
        ctx.fatal('Invalid IP address')

def configure(ctx):

    # ENV - Save options for build stage
    ctx.env.BSP = "-".join([
        ctx.options.riscv_platform, ctx.options.riscv_arch,
        ctx.options.riscv_abi
    ])
    ctx.env.DEMO = ctx.options.demo
    ctx.env.PROG = ctx.options.program
    ctx.env.PLATFORM = ctx.options.riscv_platform
    ctx.env.RISCV_XLEN = ctx.options.riscv_arch[2:4]
    ctx.env.PURECAP = True if 'cheri' in ctx.options.riscv_arch else False
    ctx.env.ARCH = 'riscv' + ctx.env.RISCV_XLEN
    ctx.env.MARCH = ctx.options.riscv_arch
    ctx.env.MABI = ctx.options.riscv_abi
    ctx.env.TARGET = ctx.env.ARCH + '-unknown-elf'
    ctx.env.SYSROOT = ctx.options.sysroot
    ctx.env.MEMSTART = ctx.options.mem_start
    ctx.env.VIRTIO_BLK = ctx.options.use_virtio_blk
    ctx.env.CREATE_DISK_IMAGE = ctx.options.create_disk_image
    ctx.env.PROGRAM_PATH = ctx.options.program_path
    ctx.env.PROGRAM_ENTRY = ctx.env.PROG
    ctx.env.COMPARTMENTALIZE = ctx.options.compartmentalize
    ctx.env.COMP_MODE = ctx.options.compartmentalization_mode
    ctx.env.DEBUG = ctx.options.debug
    ctx.env.IP_ADDR = ctx.options.ipaddr
    ctx.env.GATEWAY_ADDR = ctx.options.gateway

    ipaddr_freertos_ipconfig(ctx.env.IP_ADDR, ctx.env.GATEWAY_ADDR, ctx)

    # Libs - Minimal libs required for any FreeRTOS Demo
    ctx.env.append_value('LIB', ['c'])
    ctx.env.append_value('LIB_DEPS', ['freertos_core', 'freertos_bsp'])

    # DEFINES - Global defines
    ctx.env.append_value('DEFINES', ['__freertos__=1'])
    ctx.env.append_value('DEFINES', ['__waf__=1'])
    ctx.env.append_value('DEFINES', ['HAVE_CONFIG_H=1'])

    # TOOLCHAIN - Check for a valid installed toolchain
    if ctx.options.toolchain == "llvm":
        ctx.env.CC = 'clang'
        ctx.env.AS = 'clang'
        ctx.env.ASM_NAME = 'clang'
        ctx.env.AS_TGT_F = ['-c', '-o']
        ctx.env.ASLNK_TGT_F = ['-o']
        ctx.env.ASFLAGS = ctx.env.CFLAGS
        ctx.env.append_value('CFLAGS', ['-target', ctx.env.TARGET])
        ctx.env.append_value('CFLAGS', '-mno-relax')
        ctx.env.append_value('CFLAGS', '-mcmodel=medium')

        ctx.env.append_value('STLIBPATH', [])
        ctx.env.append_value('LIB', 'clang_rt.builtins-' + ctx.env.ARCH)
        ctx.env.append_value('STLIBPATH', [ctx.env.SYSROOT + '/lib'])
        ctx.env.append_value('LINKFLAGS', ['-L' + ctx.env.SYSROOT + '/lib'])

        ctx.env.append_value('LDFLAGS', ['-fuse-ld=lld'])

        ctx.env.append_value('INCLUDES', [ctx.env.SYSROOT + '/include'])

    elif ctx.options.toolchain == "gcc":
        ctx.env.CC = ctx.env.TARGET + '-gcc'
        ctx.env.AS = ctx.env.TARGET + '-gcc'
        ctx.env.LD = ctx.env.TARGET + '-gcc'
        ctx.env.append_value('LIB', ['gcc'])
        ctx.env.append_value('CFLAGS', '-mcmodel=medany')
    else:
        ctx.fatal("Invalid toolchain")

    try:
        ctx.load('compiler_c')
        ctx.load('gas')
    except:
        ctx.fatal("Invalid toolchain")

    # PURECAP
    if ctx.options.purecap and not ctx.env.PURECAP:

        ctx.env.append_value('DEFINES', ['CONFIG_ENABLE_CHERI=1'])

        ctx.define('CONFIG_ENABLE_CHERI', 1)

        ctx.env.PURECAP = True

        if ctx.options.toolchain != "llvm":
            ctx.fatal("Purecap mode is only supported by LLVM")

        ctx.env.MARCH += "xcheri"

        if ctx.env.RISCV_XLEN == "64":
            ctx.env.MABI = "l64pc128"
        elif ctx.env.RISCV_XLEN == "32":
            ctx.env.MABI = "il32pc64"
        else:
            ctx.fatal("RISCV_XLEN not supporte for PURECAP")

    if ctx.env.PURECAP:
        ctx.env.append_value('LIB_DEPS', ["cheri"])
        ctx.env.append_value('CFLAGS', ['-Werror=cheri-prototypes'])

    if ctx.env.PROGRAM_PATH and ctx.path.find_resource(ctx.env.PROGRAM_PATH +
                                                       '/wscript'):
        ctx.recurse(ctx.env.PROGRAM_PATH)

    # FreeRTOS has itw own non-libc file system
    ctx.define('_STAT_H_', 1)

    if ctx.env.COMPARTMENTALIZE:
        # This define is used if FreeRTOS is configured/built with dynamic loading support
        ctx.define('configLIBDL_PROG_START_OBJ', '/lib/lib'+ctx.env.PROG+'.a:'+ctx.env.LIBDL_PROG_START_FILE+'.1.o')
        ctx.define('configPORT_ALLOW_APP_EXCEPTION_HANDLERS', 1)
        ctx.define('mainRAM_DISK_NAME', "/")
        ctx.define('configCHERI_COMPARTMENTALIZATION', 1)
        ctx.define('ipconfigUSE_FAT_LIBDL', 1)
        ctx.define('ffconfigMAX_FILENAME', 255)
        ctx.define('mainCONFIG_INIT_FAT_FILESYSTEM', 1)
        ctx.define('mainCONFIG_USE_DYNAMIC_LOADER', 1)
        ctx.define('configEMBED_LIBS_FAT', 1)
        ctx.define('configLIBDL_LIB_PATH',"/lib/")
        ctx.define('configLIBDL_CONF_PATH', "/etc/")
        ctx.define('configCOMPARTMENTS_NUM', 1024)
        ctx.define('configMAXLEN_COMPNAME', 255)

        # Compartmentalizaion requires the dynamic loader/linker lib and a filesystem
        ctx.env.append_value('LIB_DEPS', ['freertos_libdl', 'freertos_fat'])

        if ctx.env.COMP_MODE == "objs":
            ctx.define('configCHERI_COMPARTMENTALIZATION_MODE', 1)
        elif ctx.env.COMP_MODE == "libs":
            ctx.define('configCHERI_COMPARTMENTALIZATION_MODE', 2)
        else:
            ctx.fatal('Invalid compartmentalization mode: either objs or libs are supported')
    else:
        ctx.define('configCOMPARTMENTS_NUM', 0)
        ctx.define('configMAXLEN_COMPNAME', 0)

    if ctx.env.CREATE_DISK_IMAGE:
        try:
            ctx.find_program('mtools')
        except ctx.errors.ConfigurationError:
            ctx.to_log('mtools program is required to be installed to create disk images')

        ctx.define('configFF_FORMATTED_DISK_IMAGE', 1)

    # CFLAGS - Shared required CFLAGS
    ctx.env.append_value(
        'CFLAGS',
        ['-march=' + ctx.env.MARCH, '-mabi=' + ctx.env.MABI])

    if ctx.env.DEBUG:
        ctx.env.append_value('CFLAGS', ['-g', '-O0'])
        ctx.define('configPORT_ALLOW_APP_EXCEPTION_HANDLERS', 1)
        ctx.define('DEBUG', 1)
    else:
        ctx.env.append_value('CFLAGS', ['-Os'])
        ctx.define('NDEBUG', 1)



    # PROG - For legacy compatibility
    if not any('configPROG_ENTRY' in define for define in ctx.env.DEFINES):
        ctx.env.append_value('DEFINES', ['configPROG_ENTRY=' + ctx.env.PROG])

    freertos_bsp_configure(ctx)

    ctx.write_config_header('waf_config.h')

# Copied from https://nachtimwald.com/2019/10/09/python-binary-to-c-header/
def bin2header(data, var_name='var'):
    out = []
    out.append('unsigned char {var_name}[] = {{'.format(var_name=var_name))
    l = [data[i:i + 12] for i in range(0, len(data), 12)]
    for i, x in enumerate(l):
        line = ', '.join(['0x{val:02x}'.format(val=ord(c)) for c in x])
        out.append('  {line}{end_comma}'.format(
            line=line, end_comma=',' if i < len(l) - 1 else ''))
    out.append('};')
    out.append('unsigned int {var_name}_len = {data_len};'.format(
        var_name=var_name, data_len=len(data)))
    return '\n'.join(out)


# Generate a header that containts hex content of the libdl libs to be embedded
# in the file system
def gen_header_libs(bld):
    # Add the main app to be embedded
    LIBS_TO_EMBED = ["lib" + lib + ".a" for lib in bld.env.LIB_DEPS_EMBED_FAT]

    # Make sure all libraries are built and ready
    for lib in bld.env.LIB_DEPS_EMBED_FAT:
        tg = bld.get_tgen_by_name(lib)
        tg.post()
        #for task in tg.tasks:
            #for obj_file in task.outputs:
                #if bld.env.PROG in str(obj_file):
                #    print(str(obj_file).split('/')[-1])
                #    bld.env.append_value(
                #        'DEFINES',
                #        'mainCONFIG_USE_DYNAMIC_LOADER_START_OBJ = ' +
                #        str(obj_file).split('/')[-1])
            #print(task.outputs)
        for task in tg.tasks:
            task.run()

    # Create a libdl.conf file that contains the list of libraries
    libdl_config = '\n'.join(['/lib/' + lib for lib in LIBS_TO_EMBED])

    # Defines vars and types used by CreateAndVerifyExampleFiles.c
    header_content = """
typedef struct LIBFILE_TO_COPY {
  const char *pcFileName;
  size_t xFileSize;
  const uint8_t *pucFileData;
} xLibFileToCopy_t;
                      """
    # Convert files to hex arrays
    for lib in LIBS_TO_EMBED:
        lib_path = str(bld.path.get_bld().ant_glob('**/' + lib, quiet=True)[0])
        lib = lib.replace('.', '_')
        with open(lib_path, 'r') as f:
            data = f.read()
            header_content += (bin2header(data, lib)) + '\n'

    # Write the libdl.conf hex
    header_content += bin2header(libdl_config, 'libdl_conf') + '\n'

    header_content += """
const xLibFileToCopy_t xLibFilesToCopy[] = {
    """

    for lib in LIBS_TO_EMBED:
        header_content += '{ "/lib/' + lib + '", sizeof(' + lib.replace(
            '.', '_') + '), ' + lib.replace('.', '_') + ' },\n'

    header_content += '{ "/etc/libdl.conf", sizeof(libdl_conf), libdl_conf} '
    header_content += '};'

    # Write the final content to the header file
    with open(str(bld.path.get_bld()) + "/libs_fat_embedded.h", 'w') as f:
        f.write(header_content)

def build(bld):

    # Init FreeRTOS bsps, libs, and demos
    freertos_init(bld)

    # Final ELF target name
    PROG_NAME = bld.env.DEMO + "_" + bld.env.PROG + ".elf"

    # TODO
    comp = Compartmentalize(env=bld.env)
    bld.add_to_group(comp)

    # LIBS - Build required libs
    for lib in bld.env.LIB_DEPS:
        if lib in bld.env.LIB_DEPS_EMBED_FAT:
            bld.env.libs[lib].is_compartment = True
        bld.env.libs[lib].build(bld)

    # PROG - FreeRTOS Program
    if bld.env.PROGRAM_PATH and bld.path.find_resource(bld.env.PROGRAM_PATH +
                                                       '/wscript'):
        # The program defines its own script
        try:
            bld.recurse(bld.env.PROGRAM_PATH)
        except:
            bld.fatal('Bad provided program path and/or wscript')
    else:
        # Just for legacy compatibility where simple programs with one file are assumed
        # to have main_xxx.c files under demo/ and don't define their own wscript.
        bld.stlib(source=['./demo/' + bld.env.PROG + '.c'],
                  target=bld.env.PROG)

    main_sources = ['main.c']
    main_libs = bld.env.LIB_DEPS + [bld.env.PROG]

    # LIBS - wrap libs in groups for circular deps (mainly for gcc)
    bld.env.STLIB_MARKER = ['-Wl,-Bstatic', '-Wl,--start-group']

    if bld.env.COMPARTMENTALIZE:

        # Requested-to-be-compartmentalized libraries are in LIB_DEPS_EMBED_FAT
        # Add the application to them
        bld.env.LIB_DEPS_EMBED_FAT += [bld.env.PROG]
        for embed_lib in bld.env.LIB_DEPS_EMBED_FAT:
            if embed_lib in bld.env.LIB_DEPS:
                bld.env.LIB_DEPS.remove(embed_lib)

        # Generate a header with hex contet of the libraries and embed/write them
        # in run-time. If ctx.env.CREATE_DISK_IMAGE is true, then the image will
        # be created on the host and ready to used by FreeRTOS without the need
        # to parition, format or write any libs in run-time.
        if not bld.env.CREATE_DISK_IMAGE:
            # Generate the libs_fat_embedded.h header with libs
            gen_header_libs(bld)
            # Build the C files that embeds the libs into the FAT filesystem
            main_sources += ['libs_fat_embedded.c']

        # Need to include all unused functions because a future dynamically loaded
        # object might want to link against it
        bld.env.append_value('STLIB_MARKER', ['-Wl,--whole-archive'])

        # Compartmentalization needs libdl + fat
        if 'freertos_fat' not in bld.env.LIB_DEPS:
            main_libs += ['freertos_fat', 'freertos_fat_headers']
        if 'freertos_libdl' not in bld.env.LIB_DEPS:
            main_libs += ['freertos_libdl', 'freertos_libdl_headers']

    bld.env.SHLIB_MARKER = ['-Wl,--no-whole-archive', '-lc', '-Wl,--end-group']

    bld.env.append_value('STLIB', main_libs)
    # Remoeve any lib duplicates
    bld.env.STLIB = list(dict.fromkeys(bld.env.STLIB))
    # Remoeve dynamically loaded libs
    bld.env.STLIB = [lib for lib in bld.env.STLIB if lib not in bld.env.LIB_DEPS_EMBED_FAT]
    # Make sure all libs are built (manualy dep)
    for lib in bld.env.STLIB:
        tg = bld.get_tgen_by_name(lib)
        tg.post()
        for task in tg.tasks:
            task.run()

    bld.program(
        source=main_sources,
        target=PROG_NAME,
        features="c",
        includes=['.'],
        libpath=['.', bld.env.PROGRAM_PATH],
        ldflags=bld.env.CFLAGS +
            ['-T',
            bld.path.abspath() + '/link.ld', '-nostartfiles', '-nostdlib',
            '-Wl,--defsym=MEM_START=' + str(bld.env.MEMSTART),
            '-defsym=_STACK_SIZE=4K'
        ],
    )

    bld.add_post_fun(post_build)


def post_build(ctx):

    print('After the build is complete')

    if ctx.cmd == 'install':
        if ctx.env.VIRTIO_BLK:
            create_disk_image(ctx, 5)

    # TODO
    if ctx.options.run:
        ctx.exec_command('echo "Running"')

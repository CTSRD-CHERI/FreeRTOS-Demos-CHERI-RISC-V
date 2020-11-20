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

        self.defines = [
            'PLATFORM_QEMU_VIRT          =     1',
            'configCLINT_BASE_ADDRESS    =     0x2000000',
            'configUART16550_BASE        =     0x10000000',
            'configUART16550_BAUD        =     115200LL',
            'configUART16550_REGSHIFT    =     1',
            'configCPU_CLOCK_HZ          =     10000000',
            'configPERIPH_CLOCK_HZ       =     10000000',
            'QEMU_VIRT_NET_MMIO_ADDRESS  =     0x10008000',
            'QEMU_VIRT_NET_MMIO_SIZE     =     0x1000',
            'QEMU_VIRT_NET_PLIC_INTERRUPT_ID   = 0x8',
            'QEMU_VIRT_NET_PLIC_INTERRUPT_PRIO = 0x1',
            'PLIC_BASE_ADDR              =     0xC000000ULL',
            'PLIC_NUM_SOURCES            =     127',
            'PLIC_NUM_PRIORITIES         =     7'
        ]


class FreeRTOSBspSpike(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "spike"

        self.srcs = ['./bsp/htif.c']

        self.defines = [
            'PLATFORM_SPIKE              =     1',
            'configCLINT_BASE_ADDRESS    =     0x2000000'
        ]


class FreeRTOSBspSail(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "sail"

        self.srcs = ['./bsp/htif.c']

        self.defines = [
            'PLATFORM_SAIL               =     1',
            'configCLINT_BASE_ADDRESS    =     0x2000000'
        ]


class FreeRTOSBspPiccolo(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "piccolo"

        self.srcs = ['./bsp/uart16550.c']

        self.defines = [
            'PLATFORM_PICCOLO            =     1',
            'configCLINT_BASE_ADDRESS    =     0x2000000',
            'configUART16550_BASE        =     0xC0000000',
            'configUART16550_BAUD        =     115200LL',
            'configUART16550_REGSHIFT    =     1',
        ]


class FreeRTOSBspGfe(FreeRTOSBsp):
    def __init__(self, ctx):
        self.platform = "gfe"
        self.bld_ctx = ctx,

        ctx.env.MEMSTART = 0xC0000000

        self.srcs = ['./bsp/uart16550.c']

        self.defines = [
            'PLATFORM_GFE                =     1',
            'configCLINT_BASE_ADDRESS    =     0x10000000',
            'CLINT_CTRL_ADDR             =     0x10000000',
            'configUART16550_BASE        =     0x62300000ULL',
            'configUART16550_BAUD        =     115200LL',
            'configUART16550_REGSHIFT    =     2',
            'configCPU_CLOCK_HZ          =     100000000',
            'configPERIPH_CLOCK_HZ       =     100000000',
            'PLIC_BASE_ADDR              =     0xC000000ULL',
            'PLIC_NUM_SOURCES            =     16',
            'PLIC_NUM_PRIORITIES         =     16',
            'PLIC_SOURCE_UART0           =     0x1',
            'PLIC_SOURCE_ETH             =     0x2',
            'PLIC_SOURCE_DMA_MM2S        =     0x3',
            'PLIC_SOURCE_DMA_S2MM        =     0x4',
            'PLIC_SOURCE_SPI0            =     0x5',
            'PLIC_SOURCE_UART1           =     0x6',
            'PLIC_SOURCE_IIC0            =     0x7',
            'PLIC_SOURCE_SPI1            =     0x8',
            'PLIC_PRIORITY_UART0         =     0x1',
            'PLIC_PRIORITY_ETH           =     0x2',
            'PLIC_PRIORITY_DMA_MM2S      =     0x3',
            'PLIC_PRIORITY_DMA_S2MM      =     0x3',
            'PLIC_PRIORITY_SPI0          =     0x3',
            'PLIC_PRIORITY_UART1         =     0x1',
            'PLIC_PRIORITY_IIC0          =     0x3',
            'PLIC_PRIORITY_SPI1          =     0x4',
            'MCAUSE_EXTERNAL_INTERRUPT   =' + str(0x800000000000000b)
            if ctx.env.RISCV_XLEN == "64" else str(0x8000000b)
        ]


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

    def build(self, bld):
        print("Building lib ", self.name)
        bld.stlib(features=['c'],
                  asflags=bld.env.CFLAGS + bld.env.ASFLAGS,
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
            ctx.env.ARCH][ctx.env.PLATFORM]

        self.defines = self.freertos_platform.defines

        self.includes = [".", self.freertos_bsp_dir]

        self.export_includes = [".", self.freertos_bsp_dir]

        self.srcs = [
            self.freertos_bsp_dir + 'boot.S', self.freertos_bsp_dir + 'bsp.c',
            self.freertos_bsp_dir + 'rand.c', self.freertos_bsp_dir +
            'plic_driver.c', self.freertos_bsp_dir + 'syscalls.c'
        ] + self.freertos_platform.srcs

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibCheri(FreeRTOSLib):

    libcheri_dir = '../../../FreeRTOS-Labs/Source/FreeRTOS-libcheri'

    def __init__(self, ctx):
        self.name = "cheri"
        self.srcs = [self.libcheri_dir + '/cheri/cheri-riscv.c']
        self.includes = [self.libcheri_dir + '/include']
        self.export_includes = [self.libcheri_dir + '/include']

        FreeRTOSLib.__init__(self, ctx)

class FreeRTOSLibDL(FreeRTOSLib):

    libdl_dir = '../../../FreeRTOS-Labs/Source/FreeRTOS-libdl/'

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

    libvirtio_dir = '../../../FreeRTOS-Labs/Source/FreeRTOS-libvirtio/'

    def __init__(self, ctx):
        self.name = "virtio"

        self.srcs = [
            self.libvirtio_dir + 'virtio.c',
            self.libvirtio_dir + 'virtio-net.c',
            self.libvirtio_dir + 'helpers.c'
        ]

        self.includes = [self.libvirtio_dir]
        self.export_includes = [self.libvirtio_dir]

        self.defines = ['VIRTIO_USE_MMIO=1']

        FreeRTOSLib.__init__(self, ctx)


class FreeRTOSLibTCPIP(FreeRTOSLib):

    libtcpip_dir = '../../../FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP'

    def __init__(self, ctx):
        self.name = "freertos_tcpip"

        self.includes = [
            self.libtcpip_dir, self.libtcpip_dir + '/include',
            self.libtcpip_dir + '/portable/Compiler/GCC'
        ]

        self.export_includes = [
            self.libtcpip_dir, self.libtcpip_dir + '/include',
            self.libtcpip_dir + '/portable/Compiler/GCC'
        ]

        self.driver_srcs = [
            self.libtcpip_dir +
            '/portable/NetworkInterface/virtio/NetworkInterface.c'
        ] if ctx.env.PLATFORM == "qemu_virt" else []

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

    def build(self, ctx):
        ctx.objects(source=self.srcs,
                    includes=self.export_includes + ctx.env.INCLUDES,
                    export_includes=self.includes,
                    target='freertos_tcpip')


class FreeRTOSLibFAT(FreeRTOSLib):

    libfat_dir = '../../../FreeRTOS-Labs/Source/FreeRTOS-Plus-FAT/'

    def __init__(self, ctx):
        self.name = "freertos_fat"
        self.srcs = [
            self.libfat_dir + 'ff_crc.c', self.libfat_dir + 'ff_dir.c',
            self.libfat_dir + 'ff_error.c', self.libfat_dir + 'ff_fat.c',
            self.libfat_dir + 'ff_file.c', self.libfat_dir + 'ff_format.c',
            self.libfat_dir + 'ff_ioman.c', self.libfat_dir + 'ff_locking.c',
            self.libfat_dir + 'ff_memory.c', self.libfat_dir + 'ff_string.c',
            self.libfat_dir + 'ff_sys.c', self.libfat_dir + 'ff_time.c',
            self.libfat_dir + 'ff_stdio.c',
            self.libfat_dir + 'portable/common/ff_ramdisk.c'
        ]

        self.includes = [
            self.libfat_dir + '/include', self.libfat_dir + 'portable/common'
        ]

        self.export_includes = [
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


def freertos_bsps_init(bld_ctx):
    bld_ctx.env.freertos_bsps = {}

    bld_ctx.env.freertos_bsps["spike"] = FreeRTOSBspSpike(bld_ctx)
    bld_ctx.env.freertos_bsps["qemu_virt"] = FreeRTOSBspQemuVirt(bld_ctx)
    bld_ctx.env.freertos_bsps["sail"] = FreeRTOSBspSail(bld_ctx)
    bld_ctx.env.freertos_bsps["gfe"] = FreeRTOSBspGfe(bld_ctx)
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

    # BSP options
    ctx.add_option('--riscv-platform',
                   action='store',
                   default="qemu_virt",
                   help='RISC-V Platform/Board')
    ctx.add_option('--mem-start',
                   action='store',
                   default=0x80000000,
                   help='BSP platform RAM start')

    # Features options
    ctx.add_option('--compartmentalize',
                   action='store_true',
                   default=False,
                   help='Expermintal CHERI-based SW compartmentalization')

    # Run options
    ctx.add_option('--run',
                   action='store_true',
                   default=False,
                   help='Run the program after it is built')


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
    ctx.env.PROGRAM_PATH = ctx.options.program_path
    ctx.env.PROGRAM_ENTRY = ctx.env.PROG

    # Libs - Minimal libs required for any FreeRTOS Demo
    ctx.env.append_value('LIB', ['c'])
    ctx.env.append_value('LIB_DEPS', ['freertos_core', 'freertos_bsp'])

    # DEFINES - Global defines
    ctx.env.append_value('DEFINES', ['__freertos__=1'])
    ctx.env.append_value('DEFINES', ['__waf__=1'])

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
        ctx.env.append_value('STLIB', 'clang_rt.builtins-' + ctx.env.ARCH)
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

    if ctx.env.PROGRAM_PATH and ctx.path.find_resource(ctx.env.PROGRAM_PATH +
                                                       '/wscript'):
        ctx.recurse(ctx.env.PROGRAM_PATH)

    # CFLAGS - Shared required CFLAGS
    ctx.env.append_value(
        'CFLAGS',
        ['-g', '-O0', '-march=' + ctx.env.MARCH, '-mabi=' + ctx.env.MABI])

    # PROG - For legacy compatibility
    if not any('configPROG_ENTRY' in define for define in ctx.env.DEFINES):
        ctx.env.append_value('DEFINES', ['configPROG_ENTRY=' + ctx.env.PROG])


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

    # Remove freertos_tcpip from the libs as it is a collection of objects
    if "freertos_tcpip" in bld.env.LIB_DEPS:
      bld.env.LIB_DEPS.remove("freertos_tcpip")

    # DEMO - Build the final statically linked ELF demo
    bld.program(
        source='main.c',
        target=PROG_NAME,
        features="c",
        includes=['.'],
        use=bld.env.LIB_DEPS + [bld.env.PROG],
        ldflags=bld.env.CFLAGS + bld.env.LDFLAGS + ['-Wl,--start-group'] +
        ['-l' + lib for lib in bld.env.LIB_DEPS] + ['-l' + bld.env.PROG] +
        ['-Wl,--end-group'] + [
            '-T',
            bld.path.abspath() + '/link.ld.generated', '-nostartfiles',
            '-nostdlib', '-Wl,--defsym=MEM_START=' + str(bld.env.MEMSTART),
            '-defsym=_STACK_SIZE=4K'
        ],
    )

    bld.add_post_fun(post_build)


def post_build(ctx):

    print('After the build is complete')

    # TODO
    if ctx.options.run:
        ctx.exec_command('echo "Running"')

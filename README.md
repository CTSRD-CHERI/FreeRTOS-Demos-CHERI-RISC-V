# RISC-V Generic BSP

This demo is intended to be generic and simple enough to run on different RISC-V
multilibs and platforms. It has been tested on Spike, QEMU, Piccolo and Sail.
To configure the demo to run with specific libs and/or platforms, you need to
pass a BSP triple target in the ___$(PLATFORM)-$(ARCH)-$(ABI)___.

# Build

Running make will build the default BSP configuration which is __spike-rv32imac-ilp32__

```
$ make
```

## Custom BSP Builds

To build a customised BSP with different platform, arch or abi, pass the target
triple to make in a BSP variable.

```
make BSP=$(PLATFORM)-$(ARCH)-$(ABI)
```

For example, to build the demo application for spike, rv64imac and lp64 run:

```
make BSP=spike-rv64imac-lp64
```

### $(PLATFORM)

Supported platforms are: ___spike___, ___piccolo___, ___qemu_virt___ and ___sail___

### $(ARCH)

All GCC's built multilibs are supported. For instrance, rv32imac, rv64imac, etc.
Note that FreeRTOS/RISC-V doesn't support hardware floating point, so while you
can provide and build FreeRTOS with F/D extensions (e.g., rv64imafdc),
it may not work properly at run-time as the FPU context is not saved/restored.

### $(ARCH)

All GCC's supported ABIs are supported.

# Supported Compiler Toolchains

## GCC

A RISC-V [riscv-gnu-toolchain](https://github.com/riscv/riscv-gnu-toolchain) can
be built with the following configure options to be able to build
different variants of $(ARCH) and/or $(ABI)

```
$ ./configure --prefix=$RISCV_INSTALL --with-cmodel=medany --enable-multilib
```

## clang/LLVM
TODO

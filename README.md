# RISC-V Generic Demo

This demo is intended to be generic enough to run on different (CHERI-)RISC-V configurations,
multilibs, platforms, apps, etc. It has been tested on FPGA/F1, Spike, QEMU, Piccolo and Sail.

# Build
The demo builds on Linux-based systems using WAF build system. There are two ways
to build the demo: either directly with WAF, or with *_cheribuild_*. cheribuild
makes it easier to build and install all the required toolchains and dependencies
to build FreeRTOS with this demo.

To build CheriFreeRTOS (FreeRTOS with CHERI support to provide memory safety),
cheribuild knows how to build all the toolchain and dependencies with CHERI
support, so it is advised to be used rather than building them yourself.

## LLVM
The main CHERI-RISC-V development is based on LLVM. Unlike while building with
GCC, newlib (libc) and compiler-rt (corresponds to libgcc) need to be first built
as dependencies for FreeRTOS. _cheribuild_ can build both for you and it is
described below.

## Using cheribuild

1. Clone [cheribuild](git@github.com:CTSRD-CHERI/cheribuild.git) and checkout **hmka2** branch
2. Build all the dependencies cheribuild requires in the README **Pre-Build Setup** section.
3. To build this demo with all of its dependencies run:

```
./cheribuild.py freertos-baremetal-riscv64 --freertos/prog main_servers --freertos/platform qemu_virt -d
```

Change _main_servers_ with your intended demo (e.g., _aws\_ota_, _main\_blinky_, etc)

This builds and installs:
```
   llvm-native
   newlib-baremetal-riscv64
   compiler-rt-builtins-baremetal-riscv64
   freertos-baremetal-riscv64
```


Otherwise build each dependency separately:

```
$ ./cheribuild.py llvm
$ ./cheribuild.py newlib-baremetal-riscv64
$ ./cheribuild.py compiler-rt-builtins-baremetal-riscv64
$ ./cheribuild.py freertos-baremetal-riscv64 --freertos/prog main_servers --freertos/platform qemu_virt
```

4. To run a demo on QEMU:

Install QEMU if you don't have it already:

```
./cheribuild.py qemu
```

Run:

```
./cheribuild.py run-freertos-baremetal-riscv64 --run-freertos/prog main_servers
```

## Using WAF

Assuming you built and installed both newlib and compiler-rt yourself to
$PATH\_TO\_NEWLIB and $PATH\_TO\_COMPILER repectively:

```
$ LDFLAGS=-L$PATH_TO_COMPILERT ./waf configure --program-path=$PROGRAM_PATH --program=$DEMO_NAME --riscv-platform=$PLATFORM --sysroot=$PATH_TO_NEWLIB build
```

An example how to build _main\_servers_ demo (HTTP, CLI, FTP, TFTP) to run on QEMU/virt platform:

```
$ LDFLAGS=-L$PATH_TO_COMPILERT ./waf configure --program-path=demo/servers --program=main_servers --riscv-platform=qemu_virt --sysroot=$PATH_TO_NEWLIB build
```

An example how to build AWS-OTA demo (MQTT, MBedTLS, TCP/IP, OTA, etc) to run on QEMU/virt platform:
```
$ LDFLAGS=-L$PATH_TO_COMPILERT ./waf configure --program-path=coreMQTT-Agent --program=aws_ota --riscv-platform=qemu_virt --sysroot=$PATH_TO_NEWLIB build
```


## TODO
* List cheribuild's FreeRTOS options.
* Build with CHERI support.
* Build with compartmentalization support.
* Build and run oh other HW targets (e.g., F1).

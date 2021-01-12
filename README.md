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

1. Clone [cheribuild](https://github.com/CTSRD-CHERI/cheribuild) and checkout **hmka2** branch

```
$ git clone git@github.com:CTSRD-CHERI/cheribuild.git -b hmka2
$ cd cheribuild
```

2. Build all the dependencies cheribuild requires in the README **Pre-Build Setup** section.

### Vanilla RISC-V (no CHERI)
GCC and/or LLVM can be used to build vanilla RISC-V FreeRTOS:

3. To build FreeRTOS with GCC, make sure **riscv64-unknown-elf-gcc** is in your $PATH:

```
./cheribuild.py freertos-baremetal-riscv64 --freertos/prog main_servers --freertos/toolchain gcc --freertos/platform qemu_virt -d
```

Change _main_servers_ with your intended demo (e.g., _aws\_ota_, _main\_blinky_, etc)

Ignore the next steps if you don't want to build with LLVM.

4. To build this demo with LLVM with all of its dependencies run:

```
./cheribuild.py freertos-baremetal-riscv64 --freertos/prog main_servers --freertos/platform qemu_virt -d
```

Change _main_servers_ with your intended demo (e.g., _aws\_ota_, _main\_blinky_, etc)
The **-d** flag will build all dependencies.

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

5. To run a demo on QEMU:

Install QEMU if you don't have it already:

```
./cheribuild.py qemu
```

Run:

```
./cheribuild.py run-freertos-baremetal-riscv64 --run-freertos/prog main_servers
```

## Using WAF

### Building with GCC
```
$ ./waf configure --program-path=$PROGRAM_PATH --program=$DEMO_NAME --toolchain=gcc --riscv-platform=$PLATFORM build
```

An example how to build _main\_servers_ demo (HTTP, CLI, FTP, TFTP) to run on QEMU/virt platform:

```
$ ./waf configure --program-path=demo/servers --program=main_servers --toolchain=gcc --riscv-platform=qemu_virt build
```

An example how to build AWS-OTA demo (MQTT, MBedTLS, TCP/IP, OTA, etc) to run on QEMU/virt platform:
```
$ ./waf configure --program-path=coreMQTT-Agent --program=aws_ota --toolchain=gcc --riscv-platform=qemu_virt build
```

### Building with LLVM

Assuming you built and installed both newlib and compiler-rt yourself to
$PATH\_TO\_NEWLIB and $PATH\_TO\_COMPILER respectively:

```
$ LDFLAGS=-L$PATH_TO_COMPILERT ./waf configure --program-path=$PROGRAM_PATH --program=$DEMO_NAME --riscv-platform=$PLATFORM --sysroot=$PATH_TO_NEWLIB build
```

Note that **--toolchain=llvm** is the default, hence it is not passed.

An example how to build _main\_servers_ demo (HTTP, CLI, FTP, TFTP) to run on QEMU/virt platform:

```
$ LDFLAGS=-L$PATH_TO_COMPILERT ./waf configure --program-path=demo/servers --program=main_servers --riscv-platform=qemu_virt --sysroot=$PATH_TO_NEWLIB build
```

An example how to build AWS-OTA demo (MQTT, MBedTLS, TCP/IP, OTA, etc) to run on QEMU/virt platform:
```
$ LDFLAGS=-L$PATH_TO_COMPILERT ./waf configure --program-path=coreMQTT-Agent --program=aws_ota --riscv-platform=qemu_virt --sysroot=$PATH_TO_NEWLIB build
```
## Running on QEMU

```
qemu-system-riscv64 -M virt -m 2048 -nographic -bios build/RISC-V-Generic_main_servers.elf -device virtio-net-device,netdev=net0 -netdev user,id=net0,ipv6=off,hostfwd=tcp::10021-:21,hostfwd=udp::10069-:69,hostfwd=tcp::10023-:23,hostfwd=tcp::8080-:80
```

Replace the ELF file with the program you built if it is not *main_servers.elf*

# TODO
* List cheribuild's FreeRTOS options.
* Build with CHERI support.
* Build with compartmentalization support.
* Build and run oh other HW targets (e.g., F1).

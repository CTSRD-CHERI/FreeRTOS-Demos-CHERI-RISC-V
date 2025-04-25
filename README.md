# RISC-V Generic Demo

This repo is intended to be generic enough to run on different (CHERI-)RISC-V configurations,
multilibs, platforms, apps, etc. It has been tested on FPGA/F1, Spike, QEMU, Piccolo and Sail.
There are 3 modes CheriFreeRTOS could be built in and run: 
1) Vanilla RISC-V (no CHERI),
2) purecap (complete spatial memory safety via CHERI), and,
3) CompartOS (purecap with linkage-based compartmentalisation: object files,
and static libraries).

For more [research] details about CheriFreeRTOS and the demos here, see [1-4].

This repo/demo is **NOT** standalone and is expected to be a submodule part of a classical
[FreeRTOS distribution](https://github.com/CTSRD-CHERI/FreeRTOS/tree/hmka2). 

CheriFreeRTOS is a research project (output of a PhD effort [1]) and isn't production ready. It's provided as best-effort, so expect
things to not work. Any fixes/features contributions are welcome. It is not currently very actively maintained, but if you come across any issue, you could ask on Slack.

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
3. To build this demo with LLVM with all of its dependencies run:

```
./cheribuild.py freertos-baremetal-riscv64 --freertos/prog main_servers --freertos/platform qemu_virt -d
```

Change _main_servers_ with your intended demo (e.g., _aws\_ota_, _main\_blinky_, etc. See supported demos below.)
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

4. To run a demo on QEMU:

Install QEMU if you don't have it already:

```
./cheribuild.py qemu
```

Run:

```
./cheribuild.py run-freertos-baremetal-riscv64 --run-freertos/prog main_servers
```

### CHERI-RISC-V (purecap, without compartmentalisation)
3. To build this demo with LLVM with all of its dependencies run:

```
./cheribuild.py freertos-baremetal-riscv64-purecap --freertos/prog main_servers --freertos/platform qemu_virt -d
```

Change _main_servers_ with your intended demo (e.g., _aws\_ota_, _main\_blinky_, etc)
The **-d** flag will build all dependencies.

This builds and installs:
```
   llvm-native
   newlib-baremetal-riscv64-purecap
   compiler-rt-builtins-baremetal-riscv64-purecap
   freertos-baremetal-riscv64-purecap
```


Otherwise build each dependency separately:

```
$ ./cheribuild.py llvm
$ ./cheribuild.py newlib-baremetal-riscv64-purecap
$ ./cheribuild.py compiler-rt-builtins-baremetal-riscv64-purecap
$ ./cheribuild.py freertos-baremetal-riscv64-purecap --freertos/prog main_servers --freertos/platform qemu_virt
```

4. To run a demo on QEMU:

Install QEMU if you don't have it already:

```
./cheribuild.py qemu
```

Run:

```
./cheribuild.py run-freertos-baremetal-riscv64-purecap --run-freertos/prog main_servers
```

# CompartOS: CHERI Compartmentalisation Model in CheriFreeRTOS

CheriFreeRTOS implements the CompartOS compartmentalisation model described in full details here [1-3]. By default, static libraries (.a) and object files (.o) could be compartmentalised. _cheribuild_ could be passed a flag in the above commands to do so:

```
./cheribuild.py freertos-baremetal-riscv64-purecap --freertos/prog main_servers --freertos/platform qemu_virt --freertos/compartmentalize
```

Note that compartmentalisation only works in purecap mode. There’s also an adhoc implementation of FreeRTOS-MPU (using RISC-V’s PMP) as well for research purposes (to compare CHERI security versus MPU/PMP), discussed in [1-4].

# CheriFreeRTOS build options
To list the cheribuild options for CheriFreeRTOS, type:

```
➜  cheribuild git:(hmka2) ✗ ./cheribuild.py freertos-baremetal-riscv64 --help | grep "freertos"
                     [--freertos/build_system BUILD] [--freertos/toolchain TOOLCHAIN] [--freertos/demo DEMO]
                     [--freertos/prog PROG] [--freertos/platform PLATFORM] [--freertos/memstart MEMSTART]
                     [--freertos/ipaddr IPADDR] [--freertos/gateway GATEWAY]
                     [--freertos/compartmentalize | --freertos/no-compartmentalize]
                     [--freertos/compartmentalize_stdlibs | --freertos/no-compartmentalize_stdlibs]
                     [--freertos/plot_compartments | --freertos/no-plot_compartments]
                     [--freertos/loc_stats | --freertos/no-loc_stats]
                     [--freertos/compartmentalization_mode FREERTOS/COMPARTMENTALIZATION_MODE]
                     [--freertos/use_virtio_blk | --freertos/no-use_virtio_blk]
                     [--freertos/create_disk_image | --freertos/no-create_disk_image]
                     [--freertos/debug | --freertos/no-debug] [--freertos/enable_mpu | --freertos/no-enable_mpu]
                     [--freertos/log_udp | --freertos/no-log_udp] [--freertos/bsp BSP]
                     [--freertos/implicit_mem_0 | --freertos/no-implicit_mem_0] [--run/ssh-forwarding-port PORT]
                     [--run-freertos/ephemeral | --run-freertos/no-ephemeral] [--run-freertos/demo DEMO]
                     [--run-freertos/prog PROG] [--run-freertos/bsp BSP]
                     [--run-freertos/use_virtio_blk | --run-freertos/no-use_virtio_blk]
Options for target 'freertos':
  --freertos/build_system BUILD
  --freertos/toolchain TOOLCHAIN
  --freertos/demo DEMO  The FreeRTOS Demo build. (default: 'RISC-V-Generic')
  --freertos/prog PROG  The FreeRTOS program to build. (default: 'main_blinky')
  --freertos/platform PLATFORM
  --freertos/memstart MEMSTART
  --freertos/ipaddr IPADDR
  --freertos/gateway GATEWAY
  --freertos/compartmentalize, --freertos/no-compartmentalize
  --freertos/compartmentalize_stdlibs, --freertos/no-compartmentalize_stdlibs
  --freertos/plot_compartments, --freertos/no-plot_compartments
  --freertos/loc_stats, --freertos/no-loc_stats
  --freertos/compartmentalization_mode FREERTOS/COMPARTMENTALIZATION_MODE
  --freertos/use_virtio_blk, --freertos/no-use_virtio_blk
  --freertos/create_disk_image, --freertos/no-create_disk_image
  --freertos/debug, --freertos/no-debug
  --freertos/enable_mpu, --freertos/no-enable_mpu
  --freertos/log_udp, --freertos/no-log_udp
  --freertos/bsp BSP    The FreeRTOS BSP to build. This is only valid for the paramterized RISC-V-Generic. The BSP
  --freertos/implicit_mem_0, --freertos/no-implicit_mem_0
Options for target 'run-freertos':
  --run-freertos/ephemeral, --run-freertos/no-ephemeral
  --run-freertos/demo DEMO
  --run-freertos/prog PROG
  --run-freertos/bsp BSP
  --run-freertos/use_virtio_blk, --run-freertos/no-use_virtio_blk
```



# Supported platforms

CheriFreeRTOS can build and run on the following platforms (replacing $PLATFROM in "--freertos/platform $PLATFORM" with one of the following):
* **qemu_virt**: QEMU's virt machine with VirtIO, disk images, networking, etc.
* **spike**: [RISC-V's Reference Simulator](https://github.com/riscv-software-src/riscv-isa-sim) (non-CHERI).
* **sail**: Generated simulator from the [RISC-V Sail Model](https://github.com/riscv/sail-riscv) (with formal specifications) 
* **gfe-p1**: [FPGA BESSPIN SoC](https://github.com/CTSRD-CHERI/BESSPIN-GFE) from (Galois/DARPA), with a small 32-bit [CHERI-Piccolo](https://github.com/CTSRD-CHERI/Piccolo) core on VCU-118 FPGA.
* **gfe-p2**: [FPGA BESSPIN SoC](https://github.com/CTSRD-CHERI/BESSPIN-GFE) from (Galois/DARPA), with a medium [CHERI-Flute](https://github.com/CTSRD-CHERI/Flute) core on VCU-118 FPGA.
* **gfe-p3**: [FPGA BESSPIN SoC](https://github.com/CTSRD-CHERI/BESSPIN-GFE) from (Galois/DARPA), with a big out-of-order [CHERI-Toooba](https://github.com/CTSRD-CHERI/Toooba) core on VCU-118 FPGA
* **fett**: [CHERI-Toooba](https://github.com/CTSRD-CHERI/Toooba) core on Amazon's F1 cloud FPGA.

# Supported demos
CheriFreeRTOS has simple and advanced demos (with networking stack, encryption, filesystems, etc). The followings are some of them:
* **main_blinky**: Simple "Hello World" FreeRTOS program that imitates blinking a led, but uses a timer, queue, two tasks, and a console. 
* **main_servers**: Advanced demo with many networking applications and protocols such as HTTP, CLI, TCP, UDP, FAT, Telnet, etc. 
* **aws_ota**: Advanced [end-to-end OTA demo](https://docs.aws.amazon.com/freertos/latest/userguide/freertos-ota-dev.html) from [FreeRTOS/Amazon](https://github.com/FreeRTOS/coreMQTT-Agent) with protocols and libraries like MQTT, embedTLS, TCP/IP, encryption, OTA updates, communicating with S3 buckets etc.
* **cyberphys**: An ECU automotive application developed by Galois to show off security in cyberphysical systems. The CheriFreeRTOS demo of it was demonstrated in a DARPA SSITH program: [DARPA SSITH – Securing our critical systems](https://www.youtube.com/watch?v=nFmaRKwB03U&list=LL&index=2)
* **coremark**: performance benchmarking application 
* **mibench**: performance benchmarking application
* **ipc_benchmark**: custom-developed application to benchmark FreeRTOS IPC.
* **modbus**: An application that demonstrates distributed capabilities for the Modbus protocol.


# Slack
Join our [Slack channel](https://www.cl.cam.ac.uk/research/security/ctsrd/cheri/cheri-slack.html) if you have any more questions.

# References
[1] Almatary, H. (2022). CHERI compartmentalisation for embedded systems (Doctoral dissertation).

[2] Almatary, Hesham, Michael Dodson, Jessica Clarke, Peter Rugg, Ivan Gomes, Michal Podhradsky, Peter G. Neumann, Simon W. Moore, and Robert NM Watson. "CompartOS: CHERI compartmentalization for embedded systems." arXiv preprint arXiv:2206.02852 (2022).

[3] Almatary, H., Mazzinghi, A., & Watson, R. N. (2024). Case Study: Securing MMU-less Linux Using CHERI. In SE 2024-Companion (pp. 69-92). Gesellschaft für Informatik eV.

[4] [cheripedia: CompartOS-compartmentalisation](https://github.com/CTSRD-CHERI/cheripedia/wiki/CompartOS-compartmentalisation-%E2%80%90-GPREL)




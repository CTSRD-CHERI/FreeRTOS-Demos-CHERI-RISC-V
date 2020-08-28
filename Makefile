RISCV_XLEN ?= 64
RISCV_LIB  ?= elf
CCPATH =

configCPU_CLOCK_HZ ?=
configPERIPH_CLOCK_HZ ?=
configMTIME_HZ ?=

BSP ?= spike-rv32imac-ilp32
BSP_CONFIGS = $(word $2,$(subst -, ,$(BSP)))

PLATFORM ?=$(call BSP_CONFIGS,$*,1)
ARCH ?=$(call BSP_CONFIGS,$*,2)
ABI ?=$(call BSP_CONFIGS,$*,3)

MEM_START?=0x80000000

TARGET =$(CCPATH)riscv${RISCV_XLEN}-unknown-${RISCV_LIB}

TOOLCHAIN ?=gcc
SYSROOT   ?=
LIBS_PATH ?=
CFLAGS    ?=
LDFLAGS   ?=
LIBS      ?=
COMPARTMENTS = comp1.wrapped.o comp2.wrapped.o
CFLAGS = -DconfigCOMPARTMENTS_NUM=2

CFLAGS += -DconfigMAXLEN_COMPNAME=32

CFLAGS += -D__freertos__=1

EXTENSION ?=
CHERI_CLEN ?= 2*$(RISCV_XLEN)
# CHERI is only supported by LLVM/Clang
ifeq ($(EXTENSION),cheri)
TOOLCHAIN = llvm
endif

ifeq ($(TOOLCHAIN),llvm)
CC      = clang -target $(TARGET)
GCC     = $(TARGET)-$(CC)
OBJCOPY = llvm-objcopy
OBJDUMP = llvm-objdump
AR      = llvm-ar
RANLIB  = llvm-ranlib
CFLAGS  += -mno-relax -mcmodel=medium --sysroot=$(SYSROOT)
LDFLAGS += --sysroot=$(SYSROOT) -lclang_rt.builtins-riscv$(RISCV_XLEN)
LIBS	  += -lc
else
CC       = $(TARGET)-gcc
OBJCOPY  = $(TARGET)-objcopy
OBJDUMP  = $(TARGET)-objdump
AR       = $(TARGET)-ar
RANLIB   = $(TARGET)-ranlib
LIBS    += -lc -lgcc
CFLAGS  += -mcmodel=medany
LDFLAGS =
endif

# Use main_blinky as demo source and target file name if not specified
PROG 	?= main_blinky
PLATFORM ?= spike
CRT0	= bsp/boot.S

# For debugging
$(info $$PROG is [${PROG}])

FREERTOS_SOURCE_DIR	= ../../Source
FREERTOS_PLUS_SOURCE_DIR = ../../../FreeRTOS-Plus/Source
FREERTOS_TCP_SOURCE_DIR = $(FREERTOS_PLUS_SOURCE_DIR)/FreeRTOS-Plus-TCP
FREERTOS_PROTOCOLS_DIR = ./protocols
FREERTOS_FAT_SOURCE_DIR = ../../../FreeRTOS-Labs/Source/FreeRTOS-Plus-FAT
FREERTOS_CLI_SOURCE_DIR = ../../../FreeRTOS-Plus/Source/FreeRTOS-Plus-CLI/
FREERTOS_LIBCHERI_DIR = ../../../FreeRTOS-Labs/Source/FreeRTOS-libcheri
FREERTOS_LIBDL_DIR = ../../../FreeRTOS-Labs/Source/FreeRTOS-libdl
FREERTOS_DEMO_IP_PROTOCOLS_DIR = demo/servers/Common/Demo_IP_Protocols

MODBUS_DEMO_DIR = ./modbus_demo
LIBMACAROONS_DIR = $(MODBUS_DEMO_DIR)/libmacaroons
LIBMODBUS_DIR = $(MODBUS_DEMO_DIR)/libmodbus
LIBMODBUS_CHERI_DIR = $(MODBUS_DEMO_DIR)/libmodbus_cheri
LIBMODBUS_MACAROONS_DIR = $(MODBUS_DEMO_DIR)/libmodbus_macaroons

WARNINGS= -Wall -Wextra -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wunused


# Root of RISC-V tools installation
RISCV ?= /opt/riscv

FREERTOS_SRC = \
	$(FREERTOS_SOURCE_DIR)/croutine.c \
	$(FREERTOS_SOURCE_DIR)/list.c \
	$(FREERTOS_SOURCE_DIR)/queue.c \
	$(FREERTOS_SOURCE_DIR)/tasks.c \
	$(FREERTOS_SOURCE_DIR)/timers.c \
	$(FREERTOS_SOURCE_DIR)/event_groups.c \
	$(FREERTOS_SOURCE_DIR)/stream_buffer.c \
	$(FREERTOS_SOURCE_DIR)/portable/MemMang/heap_4.c # TODO TEST

APP_SOURCE_DIR	= ../Common/Minimal

PORT_SRC = $(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V/port.c
ifeq ($(EXTENSION),cheri)
PORT_ASM = $(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V/chip_specific_extensions/CHERI/portASM.S
else
PORT_ASM = $(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V/portASM.S
endif

INCLUDES = \
	-I. \
	-I./bsp \
	-I$(FREERTOS_SOURCE_DIR)/include \
	-I../Common/include \
	-I$(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V

ASFLAGS  += -g -march=$(ARCH) -mabi=$(ABI)  -Wa,-Ilegacy -I$(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V/chip_specific_extensions/RV32I_CLINT_no_extensions -DportasmHANDLE_INTERRUPT=external_interrupt_handler \
	-DconfigPORT_ALLOW_APP_EXCEPTION_HANDLERS=1

CFLAGS += $(WARNINGS) $(INCLUDES)
CFLAGS += -g -O0 -march=$(ARCH) -mabi=$(ABI)

DEMO_SRC = main.c

APP_SRC = \
	bsp/bsp.c \
	bsp/plic_driver.c \
	bsp/syscalls.c \

LIBDL_SRC = $(FREERTOS_LIBDL_DIR)/libdl/dlfcn.c \
            $(FREERTOS_LIBDL_DIR)/libdl/fastlz.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-alloc-heap.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-allocator.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-alloc-lock.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-bit-alloc.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-chain-iterator.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-elf.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-error.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-mdreloc-riscv.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-obj-cache.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-obj-comp.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-obj.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-string.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-sym.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-trace.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-unresolved.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-unwind-dw2.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl-freertos-compartments.c \
            $(FREERTOS_LIBDL_DIR)/libdl/rtl.c
CFLAGS += -I$(FREERTOS_LIBDL_DIR)/include

LIBCHERI_SRC = $(FREERTOS_LIBCHERI_DIR)/cheri/cheri-riscv.c

FREERTOS_IP_SRC = \
    $(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IP.c \
    $(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_ARP.c \
    $(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_DHCP.c \
    $(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_DNS.c \
    $(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_Sockets.c \
    $(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_IP.c \
    $(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_UDP_IP.c \
    $(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_WIN.c \
    $(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_Stream_Buffer.c \
    $(FREERTOS_TCP_SOURCE_DIR)/portable/BufferManagement/BufferAllocation_2.c \
    $(FREERTOS_TCP_SOURCE_DIR)/portable/NetworkInterface/virtio/NetworkInterface.c \
    bsp/rand.c

FREERTOS_IP_INCLUDE = \
    -I$(FREERTOS_TCP_SOURCE_DIR) \
    -I$(FREERTOS_TCP_SOURCE_DIR)/include \
    -I$(FREERTOS_TCP_SOURCE_DIR)/portable/Compiler/GCC

FREERTOS_FAT_SOURCE = \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_crc.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_dir.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_error.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_fat.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_file.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_format.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_ioman.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_locking.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_memory.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_string.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_sys.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_time.c \
    $(FREERTOS_FAT_SOURCE_DIR)/ff_stdio.c \
    $(FREERTOS_FAT_SOURCE_DIR)/portable/common/ff_ramdisk.c

FREERTOS_FAT_INCLUDE = -I$(FREERTOS_FAT_SOURCE_DIR)/include \
                       -I$(FREERTOS_FAT_SOURCE_DIR)/portable/common

FREERTOS_CLI_SRC = $(FREERTOS_CLI_SOURCE_DIR)/FreeRTOS_CLI.c
FREERTOS_CLI_INCLUDE = -I$(FREERTOS_CLI_SOURCE_DIR)

FREERTOS_DEMO_IP_FTP_SRC = demo/servers/Common/Demo_IP_Protocols/FTP/FreeRTOS_FTP_server.c \
                           demo/servers/Common/Demo_IP_Protocols/FTP/FreeRTOS_FTP_commands.c

FREERTOS_DEMO_IP_TFTP_SRC = demo/servers/Common/FreeRTOS_Plus_TCP_Demos/TFTPServer.c

FREERTOS_LIBVIRTIO_DIR = ../../../FreeRTOS-Labs/Source/FreeRTOS-libvirtio
LIBVIRTIO_SRC = \
   $(FREERTOS_LIBVIRTIO_DIR)/virtio.c \
   $(FREERTOS_LIBVIRTIO_DIR)/virtio-net.c \
   $(FREERTOS_LIBVIRTIO_DIR)/helpers.c

LIBVIRTIO_INCLUDE = -I$(FREERTOS_LIBVIRTIO_DIR)

FREERTOS_IP_DEMO_SRC = \
    demo/TCPEchoClient_SingleTasks.c \
    demo//SimpleUDPClientAndServer.c \
    demo/SimpleTCPEchoServer.c

LIBMODBUS_SRC = \
	$(LIBMODBUS_DIR)/src/modbus.c \
	$(LIBMODBUS_DIR)/src/modbus-data.c \
	$(LIBMODBUS_DIR)/src/modbus-tcp.c \
	$(LIBMODBUS_DIR)/src/modbus-helpers.c

LIBMODBUS_INCLUDES = \
	-I$(LIBMODBUS_DIR)/src \
	-I$(LIBMODBUS_DIR)/include

LIBMODBUS_CHERI_SRC = \
	$(LIBMODBUS_CHERI_DIR)/src/modbus_cheri.c

LIBMODBUS_CHERI_INCLUDES = \
	-I$(LIBMODBUS_CHERI_DIR)/include

LIBMODBUS_MACAROONS_SRC = \
	$(LIBMODBUS_MACAROONS_DIR)/src/modbus_macaroons.c

LIBMODBUS_MACAROONS_INCLUDES = \
	-I$(LIBMODBUS_MACAROONS_DIR)/include

LIBMACAROONS_SRC = \
	$(LIBMACAROONS_DIR)/src/base64.c \
	$(LIBMACAROONS_DIR)/src/explicit_bzero.c \
	$(LIBMACAROONS_DIR)/src/macaroons.c \
	$(LIBMACAROONS_DIR)/src/packet.c \
	$(LIBMACAROONS_DIR)/src/port.c \
	$(LIBMACAROONS_DIR)/src/sha256.c \
	$(LIBMACAROONS_DIR)/src/shim.c \
	$(LIBMACAROONS_DIR)/src/slice.c \
	$(LIBMACAROONS_DIR)/src/timingsafe_bcmp.c \
	$(LIBMACAROONS_DIR)/src/tweetnacl.c \
	$(LIBMACAROONS_DIR)/src/v1.c \
	$(LIBMACAROONS_DIR)/src/v2.c \
	$(LIBMACAROONS_DIR)/src/varint.c

LIBMACAROONS_INCLUDES = \
	-I$(LIBMACAROONS_DIR)/include

ifeq ($(EXTENSION),cheri)
DEMO_SRC += $(LIBCHERI_SRC)
CFLAGS += -I$(FREERTOS_LIBCHERI_DIR)/include
CFLAGS += -Werror=cheri-prototypes
endif

ifeq ($(PROG),main_blinky)
	CFLAGS += -DmainDEMO_TYPE=1
	DEMO_SRC += $(PROG).c
else
ifeq ($(PROG),main_tests)
	CFLAGS += -DmainDEMO_TYPE=3
	DEMO_SRC += $(PROG).c
else
ifeq ($(PROG),main_compartment_test)
	CFLAGS += -DmainDEMO_TYPE=4
	DEMO_SRC += $(PROG).c
	DEMO_SRC += demo/compartments/loader.c
	DEMO_SRC += comp_strtab_generated.c
	DEMO_SRC += $(LIBDL_SRC)
else
ifeq ($(PROG),main_peekpoke)
	CFLAGS += -DmainDEMO_TYPE=5
    CFLAGS += -DmainCREATE_PEEKPOKE_SERVER_TASK=1
    CFLAGS += -DmainCREATE_HTTP_SERVER=1
    CFLAGS += -DipconfigUSE_HTTP=1
    CFLAGS += '-DconfigHTTP_ROOT="/notused"'
    CFLAGS += -DffconfigMAX_FILENAME=4096
    INCLUDES += \
        $(FREERTOS_IP_INCLUDE) \
        -I$(FREERTOS_PROTOCOLS_DIR)/include
    FREERTOS_SRC += \
        $(FREERTOS_IP_SRC) \
        $(FREERTOS_PROTOCOLS_DIR)/Common/FreeRTOS_TCP_server.c \
        $(FREERTOS_PROTOCOLS_DIR)/HTTP/FreeRTOS_HTTP_server.c \
        $(FREERTOS_PROTOCOLS_DIR)/HTTP/FreeRTOS_HTTP_commands.c \
        $(FREERTOS_PROTOCOLS_DIR)/HTTP/peekpoke.c
	DEMO_SRC += $(PROG).c
    DEMO_SRC += $(FREERTOS_IP_DEMO_SRC)
else
ifeq ($(PROG),main_servers)
	CFLAGS += -DmainDEMO_TYPE=6
    CFLAGS += -DmainCREATE_HTTP_SERVER=1
    CFLAGS += -DipconfigUSE_HTTP=1
    CFLAGS += -DipconfigUSE_FTP=1
    CFLAGS += '-DconfigHTTP_ROOT="/ram/websrc/"'
    CFLAGS += -DffconfigMAX_FILENAME=256
    INCLUDES += \
        $(FREERTOS_IP_INCLUDE) \
        $(FREERTOS_FAT_INCLUDE) \
        $(FREERTOS_CLI_INCLUDE) \
        -I$(FREERTOS_DEMO_IP_PROTOCOLS_DIR)/include
    FREERTOS_SRC += \
        $(FREERTOS_IP_SRC) \
        $(FREERTOS_FAT_SOURCE) \
        $(FREERTOS_CLI_SRC) \

    DEMO_SRC += $(FREERTOS_IP_DEMO_SRC) \
        $(FREERTOS_DEMO_IP_TFTP_SRC) \
        $(FREERTOS_DEMO_IP_FTP_SRC) \
        $(FREERTOS_DEMO_IP_PROTOCOLS_DIR)/Common/FreeRTOS_TCP_server.c \
        $(FREERTOS_DEMO_IP_PROTOCOLS_DIR)/HTTP/FreeRTOS_HTTP_commands.c \
        $(FREERTOS_DEMO_IP_PROTOCOLS_DIR)/HTTP/FreeRTOS_HTTP_server.c \
        demo/servers/Common/FreeRTOS_Plus_FAT_Demos/CreateAndVerifyExampleFiles.c \
        demo/servers/Common/FreeRTOS_Plus_FAT_Demos/test/ff_stdio_tests_with_cwd.c \
        demo/servers/CLI-commands.c \
        demo/servers/Common/FreeRTOS_Plus_CLI_Demos/TCPCommandConsole.c \
        demo/servers/Common/FreeRTOS_Plus_CLI_Demos/UDPCommandConsole.c \
        demo/servers/Common/FreeRTOS_Plus_CLI_Demos/File-related-CLI-commands.c \
        demo/servers/TraceMacros/Example1/DemoIPTrace.c

    INCLUDES += -Idemo/servers -Idemo/servers/TraceMacros/Example1
    INCLUDES += -Idemo/servers/DemoTasks/include
    INCLUDES += -Idemo/servers/Common/FreeRTOS_Plus_TCP_Demos/include
    INCLUDES += -Idemo/servers/Common/FreeRTOS_Plus_CLI_Demos/include
    INCLUDES += -Idemo/servers/Common/Utilities/include
ifeq ($(PROG),modbus_baseline)
	CFLAGS += \
		-DmainDEMO_TYPE=42 \
		-D__freertos__=1 \
		-DconfigCUSTOM_HEAP_SIZE=1
	DEMO_SRC += \
		$(MODBUS_DEMO_DIR)/main_modbus.c \
		$(MODBUS_DEMO_DIR)/modbus_server.c \
		$(MODBUS_DEMO_DIR)/modbus_client.c
	INCLUDES += $(LIBMODBUS_INCLUDES)
	DEMO_SRC += $(LIBMODBUS_SRC)
else
ifeq ($(PROG),modbus_baseline_microbenchmark)
	CFLAGS += \
		-DmainDEMO_TYPE=42 \
		-D__freertos__=1 \
		-DconfigCUSTOM_HEAP_SIZE=1 \
		-DNDEBUG=1 \
		-DMICROBENCHMARK=1
	DEMO_SRC += \
		$(MODBUS_DEMO_DIR)/main_modbus.c \
		$(MODBUS_DEMO_DIR)/modbus_server.c \
		$(MODBUS_DEMO_DIR)/modbus_client.c
	INCLUDES += $(LIBMODBUS_INCLUDES)
	DEMO_SRC += $(LIBMODBUS_SRC)
else
ifeq ($(PROG),modbus_cheri_layer)
	CFLAGS += \
		-DmainDEMO_TYPE=42 \
		-DCHERI_LAYER=1 \
		-D__freertos__=1 \
		-DconfigCUSTOM_HEAP_SIZE=1
	DEMO_SRC += \
		$(MODBUS_DEMO_DIR)/main_modbus.c \
		$(MODBUS_DEMO_DIR)/modbus_server.c \
		$(MODBUS_DEMO_DIR)/modbus_client.c
	INCLUDES += \
		$(LIBMODBUS_INCLUDES) \
		$(LIBMODBUS_CHERI_INCLUDES)
	DEMO_SRC += \
		$(LIBMODBUS_SRC) \
		$(LIBMODBUS_CHERI_SRC)
else
ifeq ($(PROG),modbus_cheri_layer_microbenchmark)
	CFLAGS += \
		-DmainDEMO_TYPE=42 \
		-DCHERI_LAYER=1 \
		-D__freertos__=1 \
		-DconfigCUSTOM_HEAP_SIZE=1 \
		-DNDEBUG=1 \
		-DMICROBENCHMARK=1
	DEMO_SRC += \
		$(MODBUS_DEMO_DIR)/main_modbus.c \
		$(MODBUS_DEMO_DIR)/modbus_server.c \
		$(MODBUS_DEMO_DIR)/modbus_client.c
	INCLUDES += \
		$(LIBMODBUS_INCLUDES) \
		$(LIBMODBUS_CHERI_INCLUDES)
	DEMO_SRC += \
		$(LIBMODBUS_SRC) \
		$(LIBMODBUS_CHERI_SRC)
else
ifeq ($(PROG),modbus_macaroons_layer)
	CFLAGS += \
		-DmainDEMO_TYPE=42 \
		-DMACAROONS_LAYER=1 \
		-D__freertos__=1 \
		-DconfigCUSTOM_HEAP_SIZE=1
	DEMO_SRC += \
		$(MODBUS_DEMO_DIR)/main_modbus.c \
		$(MODBUS_DEMO_DIR)/modbus_server.c \
		$(MODBUS_DEMO_DIR)/modbus_client.c
	INCLUDES += \
		$(LIBMODBUS_INCLUDES) \
		$(LIBMODBUS_MACAROONS_INCLUDES) \
		$(LIBMACAROONS_INCLUDES)
 	DEMO_SRC += \
		$(LIBMODBUS_SRC) \
		$(LIBMODBUS_MACAROONS_SRC) \
		$(LIBMACAROONS_SRC)
else
ifeq ($(PROG),modbus_macaroons_layer_microbenchmark)
	CFLAGS += \
		-DmainDEMO_TYPE=42 \
		-DMACAROONS_LAYER=1 \
		-D__freertos__=1 \
		-DconfigCUSTOM_HEAP_SIZE=1 \
		-DNDEBUG=1 \
		-DMICROBENCHMARK=1
	DEMO_SRC += \
		$(MODBUS_DEMO_DIR)/main_modbus.c \
		$(MODBUS_DEMO_DIR)/modbus_server.c \
		$(MODBUS_DEMO_DIR)/modbus_client.c
	INCLUDES += \
		$(LIBMODBUS_INCLUDES) \
		$(LIBMODBUS_MACAROONS_INCLUDES) \
		$(LIBMACAROONS_INCLUDES)
 	DEMO_SRC += \
		$(LIBMODBUS_SRC) \
		$(LIBMODBUS_MACAROONS_SRC) \
		$(LIBMACAROONS_SRC)
else
ifeq ($(PROG),modbus_cheri_macaroons_layers)
	CFLAGS += \
		-DmainDEMO_TYPE=42 \
		-DMACAROONS_LAYER=1 \
		-DCHERI_LAYER=1 \
		-D__freertos__=1 \
		-DconfigCUSTOM_HEAP_SIZE=1
	DEMO_SRC += \
		$(MODBUS_DEMO_DIR)/main_modbus.c \
		$(MODBUS_DEMO_DIR)/modbus_server.c \
		$(MODBUS_DEMO_DIR)/modbus_client.c
	INCLUDES += \
		$(LIBMODBUS_INCLUDES) \
		$(LIBMODBUS_CHERI_INCLUDES) \
		$(LIBMODBUS_MACAROONS_INCLUDES) \
		$(LIBMACAROONS_INCLUDES)
 	DEMO_SRC += \
		$(LIBMODBUS_SRC) \
		$(LIBMODBUS_CHERI_SRC) \
		$(LIBMODBUS_MACAROONS_SRC) \
		$(LIBMACAROONS_SRC)
else
ifeq ($(PROG),modbus_cheri_macaroons_layers_microbenchmark)
	CFLAGS += \
		-DmainDEMO_TYPE=42 \
		-DMACAROONS_LAYER=1 \
		-DCHERI_LAYER=1 \
		-D__freertos__=1 \
		-DconfigCUSTOM_HEAP_SIZE=1 \
		-DNDEBUG=1 \
		-DMICROBENCHMARK
	DEMO_SRC += \
		$(MODBUS_DEMO_DIR)/main_modbus.c \
		$(MODBUS_DEMO_DIR)/modbus_server.c \
		$(MODBUS_DEMO_DIR)/modbus_client.c
	INCLUDES += \
		$(LIBMODBUS_INCLUDES) \
		$(LIBMODBUS_CHERI_INCLUDES) \
		$(LIBMODBUS_MACAROONS_INCLUDES) \
		$(LIBMACAROONS_INCLUDES)
 	DEMO_SRC += \
		$(LIBMODBUS_SRC) \
		$(LIBMODBUS_CHERI_SRC) \
		$(LIBMODBUS_MACAROONS_SRC) \
		$(LIBMACAROONS_SRC)
>>>>>>> 0b3b17fe5... modbus: create microbenchmark versions of all apps
else
	$(info unknown demo: $(PROG))
endif # main_blinky
endif # main_tests
endif # main_compartment_test
endif # main_peekpoke
endif # main_servers
endif # modbus_baseline
endif # modbus_baseline_microbenchmark
endif # modbus_cheri_layer
endif # modbus_cheri_layer_microbenchmark
endif # modbus_macaroons_layer
endif # modbus_macaroons_layer_microbenchmark
endif # modbus_cheri_macaroons_layers
endif # modbus_cheri_macaroons_layers_microbenchmark

# PLATFORM Variants
ifeq ($(PLATFORM),spike)
	CFLAGS += -DPLATFORM_SPIKE=1
	APP_SRC += bsp/htif.c
else
ifeq ($(PLATFORM),piccolo)
	CFLAGS += -DPLATFORM_PICCOLO=1
	APP_SRC += bsp/uart16550.c
else
ifeq ($(PLATFORM),sail)
	CFLAGS += -DPLATFORM_SAIL=1
	APP_SRC += bsp/htif.c
else
ifeq ($(PLATFORM),qemu_virt)
	CFLAGS += -DPLATFORM_QEMU_VIRT=1
	CFLAGS += -DVTNET_LEGACY_TX=1
	CFLAGS += -DVIRTIO_USE_MMIO=1
	APP_SRC += bsp/uart16550.c
	APP_SRC += bsp/sifive_test.c
	FREERTOS_SRC += $(LIBVIRTIO_SRC)
	INCLUDES += $(LIBVIRTIO_INCLUDE)
else
ifeq ($(PLATFORM),rvbs)
	CFLAGS += -DPLATFORM_RVBS=1
else
ifeq ($(PLATFORM),gfe)
	CFLAGS += -DPLATFORM_GFE=1
MEM_START=0xC0000000
APP_SRC += \
	bsp/uart16550.c
else
	$(info unknown platform: $(PLATFORM))
endif
endif
endif
endif
endif
endif

ifeq ($(EXTENSION),cheri)
	CFLAGS += -DCONFIG_ENABLE_CHERI=1
	CFLAGS += -DCONFIG_CHERI_CLEN=$(CHERI_CLEN)
endif

ARFLAGS=crsv

# If configCPU_CLOCK_HZ is not empty, pass it as a definition
ifneq ($(configCPU_CLOCK_HZ),)
CFLAGS += -DconfigCPU_CLOCK_HZ=$(configCPU_CLOCK_HZ)
endif
# If configMTIME_HZ is not empty, pass it as a definition
ifneq ($(configMTIME_HZ),)
CFLAGS += -DconfigMTIME_HZ=$(configMTIME_HZ)
endif
# If configPERIPH_CLOCK_HZ is not empty, pass it as a definition
ifneq ($(configPERIPH_CLOCK_HZ),)
CFLAGS += -DconfigPERIPH_CLOCK_HZ=$(configPERIPH_CLOCK_HZ)
endif

CFLAGS += $(WARNINGS) $(INCLUDES)
CFLAGS += -O0 -g -march=$(ARCH) -mabi=$(ABI)

#
# Define all object files.
#
RTOS_OBJ = $(FREERTOS_SRC:.c=.o)
APP_OBJ  = $(APP_SRC:.c=.o)
PORT_OBJ = $(PORT_SRC:.c=.o)
DEMO_OBJ = $(DEMO_SRC:.c=.o)
PORT_ASM_OBJ = $(PORT_ASM:.S=.o)
CRT0_OBJ = $(CRT0:.S=.o)
OBJS = $(CRT0_OBJ) $(PORT_ASM_OBJ) $(PORT_OBJ) $(RTOS_OBJ) $(DEMO_OBJ) $(APP_OBJ)

LDFLAGS	+= -T link.ld.generated -nostartfiles -nostdlib -Wl,--defsym=MEM_START=$(MEM_START) -defsym=_STACK_SIZE=4K -march=$(ARCH) -mabi=$(ABI)

$(info ASFLAGS=$(ASFLAGS))
$(info LDLIBS=$(LDLIBS))
$(info CFLAGS=$(CFLAGS))
$(info LDFLAGS=$(LDFLAGS))
$(info ARFLAGS=$(ARFLAGS))

%.o: %.c
	@echo "    CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.S
	@echo "    CC $<"
	@$(CC) $(ASFLAGS) -c $(CFLAGS) -o $@ $<

all: $(PROG).elf

gen_freertos_header:
	@echo Generating FreeRTOSConfig.h header for $(PLATFORM) platform
	@sed -e 's/PLATFORM/$(PLATFORM)/g' < FreeRTOSConfig.h.in > FreeRTOSConfig.h

$(PROG).elf  : gen_freertos_header $(OBJS) Makefile
	@echo Building FreeRTOS/RISC-V for PLATFORM=$(PLATFORM) ARCH=$(ARCH) ABI=$(ABI)
	@echo Linking....
	@echo $(CFLAGS) > comp.cflags
	$(CC) -o $@ -fuse-ld=lld $(LDFLAGS) $(OBJS) $(LIBS) -v
	#@$(OBJDUMP) -S $(PROG).elf > $(PROG).asm
	@echo Completed $@

clean :
	@rm -f $(OBJS)
	@rm -f $(PROG).elf
	@rm -f $(PROG).map
	@rm -f $(PROG).asm

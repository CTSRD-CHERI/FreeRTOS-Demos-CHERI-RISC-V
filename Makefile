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
CFLAGS  = -mno-relax -mcmodel=medium --sysroot=$(SYSROOT)
LDFLAGS += --sysroot=$(SYSROOT) -lclang_rt.builtins-riscv$(RISCV_XLEN)
LIBS	  += -lc
else
CC       = $(TARGET)-gcc
OBJCOPY  = $(TARGET)-objcopy
OBJDUMP  = $(TARGET)-objdump
AR       = $(TARGET)-ar
RANLIB   = $(TARGET)-ranlib
LIBS    += -lc -lgcc
CFLAGS   = -mcmodel=medany
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
PORT_ASM = $(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V/portASM.S

INCLUDES = \
	-I. \
	-I./bsp \
	-I$(FREERTOS_SOURCE_DIR)/include \
	-I../Common/include \
	-I$(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V

ASFLAGS  += -g -march=$(ARCH) -mabi=$(ABI)  -Wa,-Ilegacy -I$(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V/chip_specific_extensions/RV32I_CLINT_no_extensions -DportasmHANDLE_INTERRUPT=external_interrupt_handler

DEMO_SRC = main.c \
	demo/$(PROG).c

APP_SRC = \
	bsp/bsp.c \
	bsp/plic_driver.c \
	bsp/syscalls.c \

INCLUDES = \
	-I. \
	-I./bsp \
	-I$(FREERTOS_SOURCE_DIR)/include \
	-I../Common/include \
	-I$(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V

ifeq ($(PROG),main_blinky)
	CFLAGS += -DmainDEMO_TYPE=1
else
	$(info unknown demo: $(PROG))
endif

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
	APP_SRC += bsp/uart16550.c
	APP_SRC += bsp/sifive_test.c
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

LDFLAGS	+= -T link.ld -nostartfiles -nostdlib -Wl,--defsym=MEM_START=$(MEM_START) -defsym=_STACK_SIZE=4K -march=$(ARCH) -mabi=$(ABI)

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
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LIBS) -v
	#@$(OBJDUMP) -S $(PROG).elf > $(PROG).asm
	@echo Completed $@

clean :
	@rm -f $(OBJS)
	@rm -f $(PROG).elf
	@rm -f $(PROG).map
	@rm -f $(PROG).asm

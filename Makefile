# Binaries
CC = arm-none-eabi-gcc

# SRC Directories
SRC_DIR = src
SUP_DIR = startup
# FreeRTOS files
SRC_FREERTOS_DIR = thirdparty/FreeRTOS-Kernel
SRC_FREERTOS_PORTABLE_DIR = thirdparty/FreeRTOS-Kernel/portable/GCC/ARM_CM4F
SRC_FREERTOS_MEMMANG_DIR = thirdparty/FreeRTOS-Kernel/portable/MemMang
# END FreeRTOS files
OBJ_DIR = obj
INC_DIR = inc
INC_DIR_CMSIS = inc/CMSIS/Core/Include
INC_DIR_CMSIS_DEV_F4 = inc/cmsis-device-f4/Include
INC_DIR_FREERTOS = thirdparty/FreeRTOS-Kernel/include
INC_DIR_FREERTOS_PORTABLE = thirdparty/FreeRTOS-Kernel/portable/GCC/ARM_CM4F
INC_DIR_FREERTOS_MEMMANG = thirdparty/FreeRTOS-Kernel/portable/MemMang
DEB_DIR = debug

# Files 
SRC := $(wildcard $(SRC_DIR)/*.c)
SRC += $(wildcard $(SUP_DIR)/*.c)
# FreeRTOS files
SRC += $(wildcard $(SRC_FREERTOS_DIR)/*.c)
SRC += $(wildcard $(SRC_FREERTOS_PORTABLE_DIR)/*.c)
SRC += $(wildcard $(SRC_FREERTOS_MEMMANG_DIR)/*.c)
# END - FreeRTOS files
OBJ := $(patsubst $(SRC_DIR)/%.c, $(SRC_DIR)/$(OBJ_DIR)/%.o, $(SRC))
OBJ := $(patsubst $(SUP_DIR)/%.c, $(SRC_DIR)/$(OBJ_DIR)/%.o, $(OBJ))
LD := $(wildcard $(SUP_DIR)/*.ld)

# FLAGS
MARCH = cortex-m4
# Standard includes first: CMSIS + STM32 device definitions provide core types/vector addresses.
# FreeRTOS include files are added with -idirafter to avoid shadowing system <stdint.h>.
CFLAGS = -g -Wall -mcpu=$(MARCH) -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -I$(INC_DIR) -I$(INC_DIR_CMSIS) -I$(INC_DIR_CMSIS_DEV_F4) -idirafter $(INC_DIR_FREERTOS) -idirafter $(INC_DIR_FREERTOS_PORTABLE) -idirafter $(INC_DIR_FREERTOS_MEMMANG) -ffreestanding -nostartfiles
# Linker settings for bare-metal: use nosys (no OS syscalls), bring in minimal libc and libgcc
# to resolve memcpy/memset and related CRT helpers called by FreeRTOS.
LFLAGS = -nostdlib -T $(LD) -Wl,-Map=$(DEB_DIR)/main.map -specs=nosys.specs -lc -lgcc

#PATHS
OPENOCD_INTERFACE = /usr/share/openocd/scripts/interface/stlink-v2.cfg
OPENOCD_TARGET = /usr/share/openocd/scripts/target/stm32f4x.cfg

# Targets
TARGET = $(DEB_DIR)/main.elf

all: $(OBJ) $(TARGET)

$(SRC_DIR)/$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c | mkobj
	$(CC) $(CFLAGS) -c -o $@ $^

$(SRC_DIR)/$(OBJ_DIR)/%.o : $(SUP_DIR)/%.c | mkobj
	$(CC) $(CFLAGS) -c -o $@ $^

$(SRC_DIR)/$(OBJ_DIR)/%.o : $(SRC_FREERTOS_DIR)/%.c | mkobj
	$(CC) $(CFLAGS) -c -o $@ $^

$(SRC_DIR)/$(OBJ_DIR)/%.o : $(SRC_FREERTOS_PORTABLE_DIR)/%.c | mkobj
	$(CC) $(CFLAGS) -c -o $@ $^

$(SRC_DIR)/$(OBJ_DIR)/%.o : $(SRC_FREERTOS_MEMMANG_DIR)/%.c | mkobj
	$(CC) $(CFLAGS) -c -o $@ $^

$(TARGET) : $(OBJ) | mkdeb
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^

mkobj:
	mkdir -p $(SRC_DIR)/$(OBJ_DIR)

mkdeb:
	mkdir -p $(DEB_DIR)

flash: FORCE
	openocd -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) &
	gdb-multiarch $(TARGET) -x $(SUP_DIR)/flash.gdb

debug: FORCE
	openocd -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) &
	gdb-multiarch $(TARGET) -x $(SUP_DIR)/debug.gdb

edit: FORCE
	vim -S Session.vim

doxy: FORCE
	cd ./docs && doxygen Doxyfile

clean: FORCE
	rm -rf $(SRC_DIR)/$(OBJ_DIR) $(DEB_DIR)

FORCE:

.PHONY = mkobj mkdeb clean FORCE flash debug edit doxy

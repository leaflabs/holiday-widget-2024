# directories
BUILD_DIR   := ./build
SRC_DIR     := ./src
INC_DIR     := ./include
HAL_SRC_DIR := ./STM32CubeL0/Drivers/STM32L0xx_HAL_Driver/Src/

# Required system.c and startup.s files by CMSIS
SYSTEM_SRC  := ./STM32CubeL0/Drivers/CMSIS/Device/ST/STM32L0xx/Source/Templates/system_stm32l0xx.c
STARTUP_SRC := ./STM32CubeL0/Drivers/CMSIS/Device/ST/STM32L0xx/Source/Templates/gcc/startup_stm32l072xx.s

# Required .c file for the printf file to work
PRINTF_SRC  := ./SmallPrintf/src/printf/printf.c

# List of all source directories to check thorugh
SRC_DIRS    := $(SRC_DIR) $(HAL_SRC_DIR)

# List of all include directories
INC_DIRS    := $(shell find $(INC_DIR) -type d) \
./STM32CubeL0/Drivers/CMSIS/Include \
./STM32CubeL0/Drivers/STM32L0xx_HAL_Driver/Inc \
./STM32CubeL0/Drivers/CMSIS/Device/ST/STM32L0xx/Include/ \
./SmallPrintf/src/

# commands
PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
AS      := $(PREFIX)as

# final filename
TARGET     := blink
TARGET_ELF := $(BUILD_DIR)/$(TARGET).elf

# linker script
LINK_MEM := linker_script.ld

# project parameters
CPU_TYPE  := cortex-m0plus
PROCESSOR_FLAG := -DSTM32L072xx

# Source files include the SRC_DIRS (except for the template files)  and the system.c and startup.s files
SRCS      := $(shell find $(SRC_DIRS) ! -name '*template.c' -name '*.c' -or -name '*.s')
SRCS      += $(SYSTEM_SRC)
SRCS	  += $(STARTUP_SRC)
SRCS      += $(PRINTF_SRC)

# Respective .o files
OBJS      := $(SRCS:%=$(BUILD_DIR)/%.o)

# Required dependency files for each .o file
DEPS      := $(OBJS:.o=.d)

INCLUDES  += $(addprefix -I,$(INC_DIRS))
ASFLAGS   += -mthumb -mcpu=$(CPU_TYPE)
CPPFLAGS  += $(INCLUDES) -MMD -MP -Wall
CFLAGS    += -mthumb -Wall -Wextra -c -mcpu=$(CPU_TYPE) \
	-std=gnu11 -Os -flto -g -gdwarf-2
LDFLAGS   += -mthumb -nostdlib -lgcc -mcpu=$(CPU_TYPE) \
	-Wl,-T,$(LINK_MEM) -Wl,--gc-sections \
	-msoft-float -Os -ffunction-sections -fdata-sections -flto -g -gdwarf-2 -Wl,--print-memory-usage

# Generated files
ANIMATION_FRAMES := $(INC_DIR)/middleware/led_matrix/animation/generated_animation_frames.h
LMATH_LUTS 		 := $(INC_DIR)/lmath/lmath_luts.h

# actual targets
.PHONY: all
all: $(ANIMATION_FRAMES) $(LMATH_LUTS) $(TARGET_ELF)

$(TARGET_ELF): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ $(PROCESSOR_FLAG)


.PHONY: clean upload
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(LMATH_LUTS)
	rm -f $(ANIMATION_FRAMES)

upload: $(TARGET_ELF)
	openocd -f interface/stlink.cfg -f target/stm32l0_dual_bank.cfg -c "program $(TARGET_ELF) verify reset exit"

# Code to build lut generating script
$(BUILD_DIR)/lut_generator : scripts/lut_generator.c
	@mkdir -p $(dir $@)
	gcc scripts/lut_generator.c -o $(BUILD_DIR)/lut_generator -Wall -lm

# Code to generate the lmath_luts.h
$(LMATH_LUTS) : $(BUILD_DIR)/lut_generator
	$(BUILD_DIR)/lut_generator > $@ || (rm -f $@; exit 1)

# Code to generate animation_frames.h
$(ANIMATION_FRAMES) : scripts/frame_generator.py
	python3 scripts/frame_generator.py > $@ || (rm -f $@; exit 1)


-include $(DEPS)

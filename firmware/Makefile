# ------------------------------------------------
# Makefile for LabDAQ-Control STM32F407 Project
# ------------------------------------------------

TARGET = LabDAQ-Control
DEBUG = 1
OPT = -Og

BUILD_DIR = build

# C sources
C_SOURCES =  \
firmware/Core/Src/main.c \
firmware/Core/Src/stm32f4xx_it.c \
firmware/Core/Src/stm32f4xx_hal_msp.c \
firmware/Core/Src/system_stm32f4xx.c \
firmware/Drivers/labdaq_mux16.c \
firmware/Drivers/labdaq_adc.c \
firmware/Drivers/labdaq_buffer.c \
firmware/Drivers/labdaq_filter.c \
firmware/Drivers/labdaq_sampler.c \
firmware/Drivers/labdaq_comm.c \
firmware/Drivers/labdaq_net.c

# ASM sources
ASM_SOURCES = 

# Binaries
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# CPU flags
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# Macros
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F407xx \
-DHSE_VALUE=25000000U

# Includes
C_INCLUDES =  \
-ICore/Inc \
-IDrivers \
-IConfig \
-Ifirmware/Core/Inc \
-Ifirmware/Drivers \
-Ifirmware/Config

# Compiler flags
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# Linker script
LDSCRIPT = firmware/STM32F407VGTx_FLASH.ld

# Libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# Default target
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-gnms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir -p $@

clean:
	-rm -fR $(BUILD_DIR)

.PHONY: all clean

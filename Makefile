########################################
## bare metal makefile for ARM Cortex ##
########################################

NAME      = hello_world
#DEBUG	  = 1

SRCS      = $(wildcard src/STM32F401XE/*.c)
SRCS  	 += $(wildcard src/*.c)

INCDIRS   = src/STM32F401XE

LSCRIPT   = src/STM32F401XE/gcc_linker.ld

DEFINES   = $(EXDEFINES)

BUILDDIR  = build/

CFLAGS    = -ffunction-sections
CFLAGS   += -mlittle-endian
CFLAGS   += -mthumb
CFLAGS   += -mcpu=cortex-m4
CFLAGS   += -mfloat-abi=hard
CFLAGS   += -mfpu=fpv4-sp-d16
CFLAGS   += -std=gnu11
CFLAGS   += -ggdb

ifdef DEBUG
    CFLAGS   += -Og
else
    CFLAGS   += -Os -flto
endif

LFLAGS    = --specs=nano.specs 
LFLAGS   += --specs=nosys.specs 
LFLAGS   += -nostartfiles
LFLAGS   += -Wl,--gc-sections
LFLAGS   += -T$(LSCRIPT)
LFLAGS   += -lm

WFLAGS    = -Wall
WFLAGS   += -Wextra
WFLAGS   += -Wstrict-prototypes
WFLAGS   += -Werror -Wno-error=unused-function -Wno-error=unused-variable
WFLAGS   += -Wfatal-errors
WFLAGS   += -Warray-bounds
WFLAGS   += -Wno-unused-parameter

GCCPREFIX = arm-none-eabi-
CC        = $(GCCPREFIX)gcc
OBJCOPY   = $(GCCPREFIX)objcopy
OBJDUMP   = $(GCCPREFIX)objdump
SIZE      = $(GCCPREFIX)size

INCLUDE   = $(addprefix -I,$(INCDIRS))

OBJS      = $(addprefix $(BUILDDIR), $(addsuffix .o, $(basename $(SRCS))))
OBJDIR    = $(sort $(dir $(OBJS)))
BIN_NAME  = $(addprefix $(BUILDDIR), $(NAME))


###########
## rules ##
###########

.PHONY: all
all: $(BIN_NAME).elf
all: $(BIN_NAME).bin
all: $(BIN_NAME).s19
all: $(BIN_NAME).hex
all: $(BIN_NAME).lst
all: print_size

.PHONY: clean
clean:
	$(RM) -r $(wildcard $(BUILDDIR)*)

###########################################
# Internal Rules                          #
###########################################

# create directories
$(OBJS): | $(OBJDIR)
$(OBJDIR):
	mkdir -p $@

# compiler
$(BUILDDIR)%.o: %.c
	@echo;
	@echo [CC] $<:
	$(CC) -MMD -c -o $@ $(INCLUDE) $(DEFINES) $(CFLAGS) $(WFLAGS) $<

# assembler
$(BUILDDIR)%.o: %.s
	@echo;
	@echo [AS] $<:
	$(CC) -c -x assembler-with-cpp -o $@ $(INCLUDE) $(DEFINES) $(CFLAGS) $(WFLAGS) $<

# linker
$(BUILDDIR)%.elf: $(OBJS)
	@echo;
	@echo [LD] $@:
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

%.bin: %.elf
	@echo;
	@echo [objcopy] $@:
	$(OBJCOPY) -O binary -S $< $@

%.s19: %.elf
	@echo;
	@echo [objcopy] $@:
	$(OBJCOPY) -O srec -S $< $@

%.hex: %.elf
	@echo;
	@echo [objcopy] $@:
	$(OBJCOPY) -O ihex -S $< $@

%.lst: %.elf
	@echo;
	@echo [objdump] $@:
	$(OBJDUMP) -D $< > $@

.PHONY: print_size
print_size: $(BIN_NAME).elf
	@echo;
	@echo [SIZE] $<:
	$(SIZE) $<


#####################
## Advanced Voodoo ##
#####################

# try to include any compiler generated dependency makefile snippet *.d
# that might exist in BUILDDIR (but don't complain if it doesn't yet).
DEPS = $(patsubst %.o,%.d,$(OBJS))
-include $(DEPS)

# make the object files also depend on the makefile itself
$(OBJS): Makefile



# project name
TARGET := project

# mcu in use (will derive hpp header from this name)
MY_MCU := stm32g031k8

# toolhchain
TOOLCHAIN_PREFIX := arm-gnu-toolchain-11.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi
CC := $(TOOLCHAIN_PREFIX)-gcc
CXX := $(TOOLCHAIN_PREFIX)-g++
OBJDUMP := $(TOOLCHAIN_PREFIX)-objdump
OBJCOPY := $(TOOLCHAIN_PREFIX)-objcopy
OBJSIZE := $(TOOLCHAIN_PREFIX)-size

# folders, files
SRCDIR := src
OBJDIR := obj
BINDIR := bin
INCLUDEDIR := include
LDSCRIPT := $(SRCDIR)/linker-script.txt
TARGETELF := $(BINDIR)/$(TARGET).elf

# flags used for C and C++
CFLAGS := -iquote$(INCLUDEDIR)
CFLAGS += -DMY_MCU_HEADER=\"$(MY_MCU).hpp\"
CFLAGS += -Icmsis
CFLAGS += -mcpu=cortex-m0plus
CFLAGS += -Os
CFLAGS += -fdata-sections
CFLAGS += -ffunction-sections
CFLAGS += -Wall
CFLAGS += -Wextra

# C++ flags
CXXFLAGS := $(CFLAGS)
CXXFLAGS += -std=c++17
CXXFLAGS += -funsigned-bitfields
CXXFLAGS += -fno-exceptions

# linker flags
LDFLAGS := -T$(LDSCRIPT)
LDFLAGS += --specs=nano.specs
LDFLAGS += -mcpu=cortex-m0plus
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(BINDIR)/$(TARGET).map.txt
LDFLAGS += -fno-exceptions

# object list (to obj dir) based on all src files
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.cpp) )

# produce an o file from cpp source file
$(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@printf "%-40s" "compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@printf "ok\n"


# default is build
default: build

# build project elf file from object files
# create lss, hex, bin files from elf file
build: $(OBJS)
	@printf "\n   %-40s" "building $(TARGETELF)"
	@$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGETELF) 
	@printf "ok\n"
	@printf "   %-40s" "creating lss, hex, bin files"
	@$(OBJDUMP) -dzC $(TARGETELF) > $(BINDIR)/$(TARGET).lss
	@$(OBJCOPY) -O ihex $(TARGETELF) $(BINDIR)/$(TARGET).hex
	@$(OBJCOPY) -O binary $(TARGETELF) $(BINDIR)/$(TARGET).bin
	@$(OBJSIZE) $(TARGETELF) > $(BINDIR)/$(TARGET).size.txt
	@printf "ok\n\n"
	@cat $(BINDIR)/$(TARGET).size.txt
	@printf "\n"

# clean and build
rebuild: clean build 

# program bin file to nucleo32 virtual drive
VIRTDIR := /media/owner/NOD_G031K8/
.PHONY: program
program: build
	@printf "   programming $(BINDIR)/$(TARGET).bin to $(VIRTDIR)..."
	@[ -e $(BINDIR)/$(TARGET).bin ] && \
    [ -e $(VIRTDIR) ] && \
    cp $(BINDIR)/$(TARGET).bin $(VIRTDIR) && \
	printf "ok\n\n" || \
    printf "failed\n\n"

# remove object files and bin folder files
.PHONY: clean
clean:
	@-rm -f obj/* 
	@-rm -f bin/*

# check to see what o files are being produced
.PHONY: check
check:
	@echo $(OBJS)

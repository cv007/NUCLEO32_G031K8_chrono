# project name
TARGET 	:= project

# mcu in use, header for mcu (a define provided to the compiler)
MY_MCU 		:= stm32g031k8
MY_MCU_HPP 	:= $(MY_MCU).hpp

# nucleo32 virtual folder for programming
VIRTDIR := /media/owner/NOD_G031K8/

# toolhchain
GCC_PRE := arm-gnu-toolchain-11.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi
CC  	:= $(GCC_PRE)-gcc
CXX 	:= $(GCC_PRE)-g++
OBJDUMP := $(GCC_PRE)-objdump
OBJCOPY := $(GCC_PRE)-objcopy
OBJSIZE := $(GCC_PRE)-size

# folders, linker script file
SRCDIR 	:= src
OBJDIR 	:= obj
BINDIR 	:= bin
INCDIR 	:= include
LSCRIPT := $(SRCDIR)/linker-script.txt

# target names
TARGETPRE := $(BINDIR)/$(TARGET)
TARGETELF := $(TARGETPRE).elf
TARGETBIN := $(TARGETPRE).bin
TARGETHEX := $(TARGETPRE).hex
TARGETSIZ := $(TARGETPRE).size.txt
TARGETLSS := $(TARGETPRE).lss
TARGETMAP := $(TARGETPRE).map.txt

# flags used for C and C++
CFLAGS := -iquote$(INCDIR)
CFLAGS += -DMY_MCU_HEADER=\"$(MY_MCU_HPP)\"
# CFLAGS += -Icmsis
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
LDFLAGS := -T$(LSCRIPT)
LDFLAGS += --specs=nano.specs
LDFLAGS += -mcpu=cortex-m0plus
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGETMAP)
LDFLAGS += -fno-exceptions

# object list (to obj dir) based on all src files
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.cpp) )

# header files list
HPPS := $(wildcard $(INCDIR)/*.hpp)

# object files require cpp source files (also compile if Makefile or header changes)
$(OBJDIR)/%.o : $(SRCDIR)/%.cpp $(HPPS) Makefile
	@printf "\t%-34s" "compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@printf "ok\n"

# elf file requires object files
$(TARGETELF) : $(OBJS) 
	@printf "\t%-34s" "building $(TARGETELF)"
	@$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGETELF) 
	@printf "ok\n"
	@printf "\t%-34s" "creating lss, hex, bin files"
	@$(OBJDUMP) -dzC $(TARGETELF) > $(TARGETLSS)
	@$(OBJCOPY) -O ihex $(TARGETELF) $(TARGETHEX)
	@$(OBJCOPY) -O binary $(TARGETELF) $(TARGETBIN)
	@$(OBJSIZE) $(TARGETELF) > $(TARGETSIZ)
	@printf "ok\n\n"
	@cat $(TARGETSIZ)
	@printf "\n"

# bin file, and others that rely on the elf file
$(TARGETBIN) : $(TARGETELF)

# default make target
default : $(TARGETELF)

# clean and build
rebuild : clean default


.PHONY : program clean

# program bin file to nucleo32 virtual drive
program : $(TARGETBIN)
	@printf "\tprogramming $(TARGETBIN) to $(VIRTDIR)..."
	@[ -e $(TARGETBIN) ] && \
    [ -e $(VIRTDIR) ] && \
    cp $(TARGETPRE).bin $(VIRTDIR) && \
	printf "ok\n\n" || \
    printf "failed\n\n"

# remove object files and bin folder files
clean :
	@printf "\t%-34s" "clean $(OBJDIR) $(BINDIR)"
	@-rm -f $(OBJDIR)/* 
	@-rm -f $(BINDIR)/*
	@printf "ok\n"


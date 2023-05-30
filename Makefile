# project name
TARGET 	:= project

# mcu in use, header for mcu (a define provided to the compiler)
MY_MCU 		:= stm32g031k8
MY_MCU_HPP 	:= $(MY_MCU).hpp

# nucleo32 virtual folder for programming
VIRTDIR := /media/owner/NOD_G031K8/

# toolhchain
GCC_PRE := arm-gnu-toolchain-11.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi
# GCC_PRE := gcc-arm-none-eabi-9-2020-q2-update/bin/arm-none-eabi
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
CFLAGS += --specs=nano.specs

# C++ flags
CXXFLAGS := $(CFLAGS)
# CXXFLAGS += -std=c++17
CXXFLAGS += -std=c++20
CXXFLAGS += -funsigned-bitfields
CXXFLAGS += -fno-exceptions
CXXFLAGS += -fmodules-ts
CXXFLAGS += -fno-rtti
CXXFLAGS += -Wno-volatile #get rid of volatile warnings for c++20

# linker flags
LDFLAGS := -T$(LSCRIPT)
LDFLAGS += --specs=nano.specs 
# LDFLAGS += --specs=nosys.specs
LDFLAGS += -mcpu=cortex-m0plus
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGETMAP)
LDFLAGS += -fno-exceptions
#will get undefined reference to `_sbrk' before the following line takes effect
#when using nano.specs, so probably not really needed
#(intended to get compile time error if malloc happens to get brought in)
LDFLAGS += -Wl,-wrap=_malloc_r


# object list (to obj dir) based on all src files
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.cpp) )

# header files list
HPPS := $(wildcard $(INCDIR)/*.hpp)

STRCPP := "   compile      "
STRELF := "   link         "
STRBIN := "   bin          "
STRRM  := "   clean        "
STRPGM := "   programming  "
STRHEX := "   hex          "

# object files require cpp source files (also compile if Makefile or header changes)
$(OBJDIR)/%.o : $(SRCDIR)/%.cpp $(HPPS) Makefile
	@printf "%s%s\n" $(STRCPP) "$<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# elf file requires object files
$(TARGETELF) : $(OBJS) 
	@printf "%s%s\n\n" $(STRELF) "$(TARGETELF)"
	@$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGETELF) 
	@$(OBJDUMP) -dzC $(TARGETELF) > $(TARGETLSS)
	@$(OBJSIZE) $(TARGETELF) > $(TARGETSIZ)
	@cat $(TARGETSIZ)
	@printf "\n"

# bin file
$(TARGETBIN) : $(TARGETELF)
	@printf "%s%s\n" $(STRBIN) "$(TARGETBIN)"
	@$(OBJCOPY) -O binary $(TARGETELF) $(TARGETBIN)

# hex file
$(TARGETHEX) : $(TARGETELF)
	@printf "%s%s\n" $(STRHEX) "$(TARGETHEX)"
	@$(OBJCOPY) -O ihex $(TARGETELF) $(TARGETHEX)


# default make target
default : $(TARGETELF)

# clean and build
rebuild : clean default


.PHONY : program clean

# program bin file to nucleo32 virtual drive
program : $(TARGETBIN)
	@printf "%s%s" $(STRPGM) "$(TARGETBIN) to $(VIRTDIR)..."
	@[ -e $(TARGETBIN) ] && \
    [ -e $(VIRTDIR) ] && \
    cp $(TARGETPRE).bin $(VIRTDIR) && \
	printf "ok\n\n" || \
    printf "failed\n\n"

# remove object files and bin folder files
clean :
	@printf "%s%s\n" $(STRRM) "$(OBJDIR)/ $(BINDIR)/"
	@-rm -f $(OBJDIR)/* 
	@-rm -f $(BINDIR)/*


# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2022
# SPDX-FileContributor: Raymond Dodge, 2025

# User config
# ===========

# ROM config
# ----------

NAME		:= first
GAME_TITLE	:= "FIRST"
GAME_CODE	:= "00"

# Defines passed to all files
# ---------------------------

DEFINES		:=

# Libraries
# ---------

LIBS		:= -lc
LIBDIRS		:= # A list of paths

# Include paths
# -------------

INCLUDES	+= source

# Tools
# -----

PREFIX		:= arm-none-eabi-
CC		:= $(PREFIX)gcc
CXX		:= $(PREFIX)g++
OBJDUMP		:= $(PREFIX)objdump
OBJCOPY		:= $(PREFIX)objcopy
NM		:= $(PREFIX)nm
MKDIR		:= mkdir
RM		:= rm -rf
FAMICONV	:= superfamiconv
PERL	:= perl
HOSTCC	:= gcc

# Verbose flag
# ------------

# `make V=` builds the binary in verbose build mode
V		:= @

# Directories
# -----------

SOURCEDIR	:= source
GRAPHICSDIR	:= graphics
SOURCEDIR_HOST	:= host
SOURCEDIR_TEST	:= test

BUILDDIR	:= build
BUILDOBJDIR	:= $(BUILDDIR)/main/objs
BUILDSRCDIR	:= $(BUILDDIR)/main/source
HOSTOBJDIR_SRC	:= $(BUILDDIR)/host/objs/src
HOSTOBJDIR_HOST	:= $(BUILDDIR)/host/objs/host
HOSTOBJDIR_TEST	:= $(BUILDDIR)/host/objs/test
HOSTEXEDIR	:= $(BUILDDIR)/host/exe

INCLUDES	+= $(BUILDSRCDIR)

# Build artfacts
# --------------

ELF		:= $(NAME).elf
DUMP		:= $(NAME).dump
ROM		:= $(NAME).gba
MAP		:= $(NAME).map
SYM		:= $(NAME).sym

GBAFIX	:= tools/gbafix/gbafix
GFXC	:= tools/gfxc/gfxc

# Source files
# ------------

SOURCES_S	:= $(wildcard $(SOURCEDIR)/*.s $(SOURCEDIR)/**/*.s)
SOURCES_C	:= $(wildcard $(SOURCEDIR)/*.c $(SOURCEDIR)/**/*.c)
SOURCES_CPP	:= $(wildcard $(SOURCEDIR)/*.cpp $(SOURCEDIR)/**/*.cpp)
SOURCES_PNG	:= $(wildcard $(GRAPHICSDIR)/*.png $(GRAPHICSDIR)/**/*.png)

HOSTSRCS_C	:= $(wildcard $(SOURCEDIR_HOST)/*.c) $(wildcard $(SOURCEDIR_HOST)/**/*.c)
TESTSRCS_C	:= $(wildcard $(SOURCEDIR_TEST)/*.c) $(wildcard $(SOURCEDIR_TEST)/**/*.c)

# Compiler and linker flags
# -------------------------

DEFINES		+= -D__GBA__

ARCH		:= -mcpu=arm7tdmi -mtune=arm7tdmi

# -Wsizeof-array-div keeps yelling whenever I need to count the number of words in an item
WARNFLAGS	:= -Wall -Wno-packed-bitfield-compat -Wno-sizeof-array-div

INCLUDEFLAGS	:= $(foreach path,$(INCLUDES),-I$(path)) \
		   $(foreach path,$(LIBDIRS),-I$(path)/include)

LIBDIRSFLAGS	:= $(foreach path,$(LIBDIRS),-L$(path)/lib)

ASFLAGS		+= -x assembler-with-cpp $(DEFINES) $(ARCH) \
		   -mthumb -mthumb-interwork $(INCLUDEFLAGS) \
		   -ffunction-sections -fdata-sections

CFLAGS		+= -std=gnu11 $(WARNFLAGS) $(DEFINES) $(ARCH) \
		   -mthumb -mthumb-interwork $(INCLUDEFLAGS) -O3 \
		   -ffunction-sections -fdata-sections -fanalyzer

CXXFLAGS	+= -std=gnu++14 $(WARNFLAGS) $(DEFINES) $(ARCH) \
		   -mthumb -mthumb-interwork $(INCLUDEFLAGS) -O3 \
		   -ffunction-sections -fdata-sections \
		   -fno-exceptions -fno-rtti

LDFLAGS		:= -mthumb -mthumb-interwork $(LIBDIRSFLAGS) \
		   -Wl,-Map,$(MAP) -Wl,--gc-sections \
		   -specs=nano.specs -T source/sys/gba_cart.ld \
		   -Wl,--start-group $(LIBS) -Wl,--end-group \
		   -Xlinker --print-memory-usage

HOSTCFLAGS += -std=gnu11
HOSTCFLAGS += $(WARNFLAGS)
HOSTCFLAGS += $(INCLUDEFLAGS)
HOSTCFLAGS += -Itest
HOSTCFLAGS += -O3
HOSTCFLAGS += -ffunction-sections
HOSTCFLAGS += -fdata-sections
HOSTCFLAGS += -fanalyzer

HOSTLDFLAGS	+= $(LIBDIRSFLAGS) \
                  -Wl,-Map,$(MAP) -Wl,--gc-sections \
                  -Wl,--start-group -lm $(LIBS) -Wl,--end-group \


# Intermediate build files
# ------------------------

OBJS		:= \
	$(patsubst $(SOURCEDIR)/%.s,$(BUILDOBJDIR)/%.s.o,$(SOURCES_S)) \
	$(patsubst $(SOURCEDIR)/%.c,$(BUILDOBJDIR)/%.c.o,$(SOURCES_C)) \
	$(patsubst $(SOURCEDIR)/%.cpp,$(BUILDOBJDIR)/%.cpp.o,$(SOURCES_CPP))

TEST_OBJS := \
	$(patsubst $(SOURCEDIR)/%.c,$(HOSTOBJDIR_SRC)/%.c.o,$(SOURCES_C)) \
	$(patsubst $(SOURCEDIR_TEST)/%.c,$(HOSTOBJDIR_TEST)/%.c.o,$(TESTSRCS_C)) \
	$(patsubst $(SOURCEDIR_HOST)/%.c,$(HOSTOBJDIR_HOST)/%.c.o,$(HOSTSRCS_C)) \

HOST_RUNNERS := \
	$(filter \
		$(HOSTEXEDIR)/test_%, \
		$(patsubst $(SOURCEDIR_TEST)/%.c,$(HOSTEXEDIR)/%,$(TESTSRCS_C)) \
	)

DEPS	:= $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# Default target
# -------

all: $(ROM)

# Rules
# -----

$(BUILDOBJDIR)/%.s.o : $(SOURCEDIR)/%.s
	@echo "  AS      $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CC) $(ASFLAGS) -MMD -MP -c -o $@ $<

$(BUILDOBJDIR)/%.c.o : $(SOURCEDIR)/%.c | generated_headers
	@echo "  CC      $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILDOBJDIR)/%.cpp.o : $(SOURCEDIR)/%.cpp | generated_headers
	@echo "  CXX     $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(HOSTOBJDIR_HOST)/%.c.o : $(SOURCEDIR_HOST)/%.c | generated_headers
	@echo "  HOSTCC  $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(HOSTCC) $(HOSTCFLAGS) -MMD -MP -c -o $@ $<

$(HOSTOBJDIR_TEST)/%.c.o : $(SOURCEDIR_TEST)/%.c | generated_headers
	@echo "  HOSTCC  $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(HOSTCC) $(HOSTCFLAGS) -MMD -MP -c -o $@ $<

$(HOSTOBJDIR_SRC)/%.c.o : $(SOURCEDIR)/%.c | generated_headers
	@echo "  HOSTCC  $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(HOSTCC) $(HOSTCFLAGS) -MMD -MP -c -o $@ $<

$(HOSTEXEDIR)/% : $(HOSTOBJDIR_TEST)/%.c.o $(HOSTOBJDIR_HOST)/bios.c.o $(HOSTOBJDIR_TEST)/harness.c.o
	@echo "  HOSTLD  $@"
	@$(MKDIR) -p $(@D)
	$(V)$(HOSTCC) -o $@ $^ $(HOSTLDFLAGS)

# Targets
# -------

# Clear the default suffixes
.SUFFIXES:
# Don't delete intermediate files
.SECONDARY:
# Delete files that weren't built properly
.DELETE_ON_ERROR:
# Secondary expansion is required for dependency variables in object rules.
.SECONDEXPANSION:

.PHONY: all clean dump sym generated_headers

$(GBAFIX): $(wildcard tools/gbafix/*.c)
	$(V)cd tools/gbafix && $(MAKE)

$(GFXC): $(wildcard tools/gfxc/*.c) $(wildcard tools/gfxc/*.cpp) $(wildcard tools/gfxc/**/*.cpp)
	$(V)cd tools/gfxc && $(MAKE)

generated_headers: $(BUILDSRCDIR)/graphics.h
OBJS += $(BUILDOBJDIR)/graphics.o
$(BUILDOBJDIR)/graphics.o $(BUILDSRCDIR)/graphics.h: $(GFXC) $(SOURCES_PNG)
	@echo "  GFXC"
	@$(MKDIR) -p $(BUILDOBJDIR)
	@$(MKDIR) -p $(BUILDSRCDIR)
	$(V)$(GFXC) $(GRAPHICSDIR) $(BUILDOBJDIR)/graphics.o $(BUILDSRCDIR)/graphics.h

generated_headers: $(BUILDSRCDIR)/graphics_types.h
$(BUILDSRCDIR)/graphics_types.h: $(GFXC)
	@echo "  GFXC    --structs"
	@$(MKDIR) -p $(BUILDSRCDIR)
	$(V)$(GFXC) --structs $(BUILDSRCDIR)/graphics_types.h

$(ELF): $(OBJS) source/sys/gba_cart.ld
	@echo "  LD      $@"
	$(V)$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(ROM): $(ELF) $(GBAFIX)
	@echo "  OBJCOPY $<"
	$(V)$(OBJCOPY) -O binary $< $@
	@echo "  GBAFIX  $@"
	$(V)$(GBAFIX) $@ -t$(GAME_TITLE) -c$(GAME_CODE)

$(HOSTEXEDIR)/test_vram_op_queue : $(HOSTOBJDIR_SRC)/management/vram_op_queue.c.o $(HOSTOBJDIR_SRC)/gba/palette.c.o $(HOSTOBJDIR_SRC)/gba/vram.c.o $(HOSTOBJDIR_SRC)/gba/oam.c.o $(HOSTOBJDIR_SRC)/gba/hw_reg.c.o

check: $(HOST_RUNNERS)
	$(V)for r in $(HOST_RUNNERS); do $$r ; done

check_all: check
	$(V)cd tools/gfxc && $(MAKE) check

$(DUMP): $(ELF)
	@echo "  OBJDUMP $@"
	$(V)$(OBJDUMP) -h -C -S $< > $@

dump: $(DUMP)

$(SYM): $(ELF)
	@echo "  OBJDUMP $@"
	$(V)$(NM) --numeric-sort --print-size $< >$@

sym: $(SYM)

clean:
	@echo "  CLEAN"
	$(V)$(RM) $(ROM) $(ELF) $(DUMP) $(SYM) $(MAP) $(BUILDDIR)
	$(V)cd tools/gbafix && $(MAKE) clean
	$(V)cd tools/gfxc && $(MAKE) clean

generated_headers:
	@:

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)

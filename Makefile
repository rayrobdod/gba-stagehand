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

INCLUDES	+= build/source
INCLUDES	+= source

# Tools
# -----

PREFIX		:= arm-none-eabi-
CC		:= $(PREFIX)gcc
CXX		:= $(PREFIX)g++
OBJDUMP		:= $(PREFIX)objdump
OBJCOPY		:= $(PREFIX)objcopy
MKDIR		:= mkdir
RM		:= rm -rf
FAMICONV	:= superfamiconv
PERL	:= perl

# Verbose flag
# ------------

# `make V=` builds the binary in verbose build mode
V		:= @

# Directories
# -----------

SOURCEDIR	:= source
GRAPHICSDIR	:= graphics
BUILDDIR	:= build
BUILDOBJDIR	:= $(BUILDDIR)/objs
BUILDSRCDIR	:= $(BUILDDIR)/source
BUILDGRAPHICSDIR	:= $(BUILDDIR)/graphics

# Build artfacts
# --------------

ELF		:= $(NAME).elf
DUMP		:= $(NAME).dump
ROM		:= $(NAME).gba
MAP		:= $(NAME).map
SYM		:= $(NAME).sym

GBAFIX		:= tools/gbafix/gbafix
GFX2OBJ		:= tools/gfx2obj/gfx2obj

# Source files
# ------------

SOURCES_S	:= $(wildcard $(SOURCEDIR)/*.s $(SOURCEDIR)/**/*.s)
SOURCES_C	:= $(wildcard $(SOURCEDIR)/*.c $(SOURCEDIR)/**/*.c)
SOURCES_CPP	:= $(wildcard $(SOURCEDIR)/*.cpp $(SOURCEDIR)/**/*.cpp)

# Compiler and linker flags
# -------------------------

DEFINES		+= -D__GBA__

ARCH		:= -mcpu=arm7tdmi -mtune=arm7tdmi

WARNFLAGS	:= -Wall

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

ICONPALFLAGS	:=
ICONTILEFLAGS	:=
ICONMAPFLAGS	:=

# Intermediate build files
# ------------------------

OBJS		:= \
	$(BUILDOBJDIR)/oldschool.png.o \
	$(patsubst $(SOURCEDIR)/%.s,$(BUILDOBJDIR)/%.s.o,$(SOURCES_S)) \
	$(patsubst $(SOURCEDIR)/%.c,$(BUILDOBJDIR)/%.c.o,$(SOURCES_C)) \
	$(patsubst $(SOURCEDIR)/%.cpp,$(BUILDOBJDIR)/%.cpp.o,$(SOURCES_CPP))

DEPS		:= $(OBJS:.o=.d)

# Default target
# -------

all: $(ROM)

# Rules
# -----

$(BUILDOBJDIR)/%.s.o : $(SOURCEDIR)/%.s
	@echo "  AS      $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CC) $(ASFLAGS) -MMD -MP -c -o $@ $<

$(BUILDOBJDIR)/%.c.o : $(SOURCEDIR)/%.c
	@echo "  CC      $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILDOBJDIR)/%.cpp.o : $(SOURCEDIR)/%.cpp
	@echo "  CXX     $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(BUILDGRAPHICSDIR)/%.png.gbapal : $(GRAPHICSDIR)/%.png
	@echo "  FAMICON PALETTE $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(FAMICONV) palette --mode gba $(ICONPALFLAGS) -d $@ -i $<

$(BUILDGRAPHICSDIR)/%.png.4bpp : $(GRAPHICSDIR)/%.png $(BUILDGRAPHICSDIR)/%.png.gbapal
	@echo "  FAMICON TILES   $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(FAMICONV) tiles --mode gba $(ICONTILEFLAGS) -d $@ -i $< -p $(BUILDGRAPHICSDIR)/$*.png.gbapal

$(BUILDGRAPHICSDIR)/%.png.tilemap : $(GRAPHICSDIR)/%.png $(BUILDGRAPHICSDIR)/%.png.gbapal $(BUILDGRAPHICSDIR)/%.png.4bpp
	@echo "  FAMICON MAP     $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(FAMICONV) map --mode gba $(ICONMAPFLAGS) -d $@ -i $< -p $(BUILDGRAPHICSDIR)/$*.png.gbapal -t $(BUILDGRAPHICSDIR)/$*.png.4bpp


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

.PHONY: all clean dump

$(GBAFIX): $(wildcard tools/gbafix/*.c)
	$(V)cd tools/gbafix && make

$(GFX2OBJ): $(wildcard tools/gfx2obj/*.c) $(wildcard tools/gfx2obj/*.cpp) $(wildcard tools/gfx2obj/*.h)
	$(V)cd tools/gfx2obj && make

$(BUILDGRAPHICSDIR)/oldschool.png.gbapal: ICONPALFLAGS := -0 FFFFFF
$(BUILDGRAPHICSDIR)/oldschool.png.4bpp: ICONTILEFLAGS := -D

$(BUILDOBJDIR)/oldschool.png.o $(BUILDSRCDIR)/oldschool.png.h: $(GFX2OBJ) $(BUILDGRAPHICSDIR)/oldschool.png.4bpp
	@echo "  GFX2OBJ oldschool.png"
	@$(MKDIR) -p $(BUILDOBJDIR)
	@$(MKDIR) -p $(BUILDSRCDIR)
	$(V)$(GFX2OBJ) tileset \
		--out_object $(BUILDOBJDIR)/oldschool.png.o \
		--out_header $(BUILDSRCDIR)/oldschool.png.h \
		--in_palettes $(BUILDGRAPHICSDIR)/oldschool.png.gbapal \
		--in_tileset $(BUILDGRAPHICSDIR)/oldschool.png.4bpp \
		--variable_name oldschool


$(ELF): $(OBJS) source/sys/gba_cart.ld
	@echo "  LD      $@"
	$(V)$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(ROM): $(ELF) $(GBAFIX)
	@echo "  OBJCOPY $<"
	$(V)$(OBJCOPY) -O binary $< $@
	@echo "  GBAFIX  $@"
	$(V)$(GBAFIX) $@ -t$(GAME_TITLE) -c$(GAME_CODE)

$(DUMP): $(ELF)
	@echo "  OBJDUMP $@"
	$(V)$(OBJDUMP) -h -C -S $< > $@

dump: $(DUMP)

$(SYM): $(ELF)
	@echo "  OBJDUMP $@"
	$(V)$(OBJDUMP) -t $< | sort -u | grep -E "^0[2356789]" | $(PERL) -p -e 's/^(\w{8}) (\w).{6} \S+\t(\w{8}) (\S+)$$/\1 \2 \3 \4/g' > $@

sym: $(SYM)

clean:
	@echo "  CLEAN"
	$(V)$(RM) $(ROM) $(ELF) $(DUMP) $(SYM) $(MAP) $(BUILDDIR)
	$(V)cd tools/gbafix && make clean
	$(V)cd tools/gfx2obj && make clean

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)

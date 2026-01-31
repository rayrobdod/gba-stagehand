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
HOSTREPORTDIR	:= $(BUILDDIR)/host/report
TESTOBJDIR	:= $(BUILDDIR)/test/objs
TESTEXEDIR	:= $(BUILDDIR)/test/exe
TESTREPORTDIR	:= $(BUILDDIR)/test/report

INCLUDES	+= $(BUILDSRCDIR)

# Build artfacts
# --------------

ELF		:= $(NAME).elf
DUMP		:= $(NAME).dump
ROM		:= $(NAME).gba
MAP		:= $(NAME).map
SYM		:= $(NAME).sym

DMGNOTES	:= tools/dmg_notes/dmg_notes
GBAFIX	:= tools/gbafix/gbafix
GFXC	:= tools/gfxc/gfxc
METADATA	:= tools/metadata/metadata
ROMTEST	?= ../icwic/tools/mgba/mgba-rom-test

# Source files
# ------------

SOURCES_S	:= $(wildcard $(SOURCEDIR)/*.s $(SOURCEDIR)/**/*.s)
SOURCES_C	:= $(wildcard $(SOURCEDIR)/*.c $(SOURCEDIR)/**/*.c)
SOURCES_CPP	:= $(wildcard $(SOURCEDIR)/*.cpp $(SOURCEDIR)/**/*.cpp)
SOURCES_PNG	:= $(wildcard $(GRAPHICSDIR)/*.png $(GRAPHICSDIR)/**/*.png $(GRAPHICSDIR)/**/**/*.png)
SOURCES_TILEDMAP	:= $(wildcard $(GRAPHICSDIR)/*.tmx $(GRAPHICSDIR)/**/*.tmx $(GRAPHICSDIR)/**/**/*.tmx)
SOURCES_TILEDSET	:= $(wildcard $(GRAPHICSDIR)/*.tsx $(GRAPHICSDIR)/**/*.tsx $(GRAPHICSDIR)/**/**/*.tsx)

HOSTSRCS_C	:= $(wildcard $(SOURCEDIR_HOST)/*.c) $(wildcard $(SOURCEDIR_HOST)/**/*.c)
TESTSRCS_C	:= $(wildcard $(SOURCEDIR_TEST)/*.c) $(wildcard $(SOURCEDIR_TEST)/**/*.c)

# Compiler and linker flags
# -------------------------

DEFINES		+= -D__GBA__

ARCH		:= -mcpu=arm7tdmi -mtune=arm7tdmi

WARNFLAGS	+= -Werror
WARNFLAGS	+= -Wall
WARNFLAGS	+= -Wextra
WARNFLAGS	+= -Wno-packed-bitfield-compat
# -Wsizeof-array-div keeps yelling whenever I need to count the number of words in an item
WARNFLAGS	+= -Wno-sizeof-array-div
WARNFLAGS	+= -Wno-missing-field-initializers

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
		   -Wl,-Map,$(MAP) \
		   -Wl,--gc-sections \
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
HOSTCFLAGS += -ggdb -DDEBUG

HOSTLDFLAGS	+= $(LIBDIRSFLAGS) \
                  -Wl,--gc-sections \
                  -Wl,--start-group -lm $(LIBS) -Wl,--end-group \

TESTLDFLAGS		:= -mthumb -mthumb-interwork $(LIBDIRSFLAGS) \
		   -Wl,--gc-sections \
		   -specs=nano.specs \
		   -T source/sys/gba_cart.ld \
		   -Wl,--start-group $(LIBS) -Wl,--end-group \

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
	$(patsubst $(SOURCEDIR_TEST)/%.c,$(TESTOBJDIR)/%.c.o,$(TESTSRCS_C)) \

HOST_RUNNERS := \
	$(filter-out \
		$(HOSTEXEDIR)/test_decompress \
		, \
		$(filter \
			$(HOSTEXEDIR)/test_% \
			, \
			$(patsubst $(SOURCEDIR_TEST)/%.c,$(HOSTEXEDIR)/%,$(TESTSRCS_C)) \
		) \
	)

TEST_REPORTS := \
	$(patsubst $(HOSTEXEDIR)/%,$(HOSTREPORTDIR)/%.txt,$(HOST_RUNNERS))

TEST_RUNNERS := \
	$(filter \
		$(TESTEXEDIR)/test_% \
		$(TESTEXEDIR)/bench_% \
		, \
		$(patsubst $(SOURCEDIR_TEST)/%.c,$(TESTEXEDIR)/%.elf,$(TESTSRCS_C)) \
	)

TEST_REPORTS := \
	$(patsubst $(TESTEXEDIR)/%.elf,$(TESTREPORTDIR)/%.txt,$(TEST_RUNNERS))

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

$(HOSTREPORTDIR)/%.txt : $(HOSTEXEDIR)/%
	@echo "  CALL    $<"
	@$(MKDIR) -p $(@D)
	$(V)bash -c "set -o pipefail; $< | tee $@"

$(TESTOBJDIR)/%.c.o : $(SOURCEDIR_TEST)/%.c | generated_headers
	@echo "  CC      $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(TESTOBJDIR)/%.png.o : $(GRAPHICSDIR)/%.png $(GFXC)
	@echo "  GFXC    --decompression_suite $<"
	@$(MKDIR) -p $(@D)
	$(V)$(GFXC) --decompression_suite $< $@

$(TESTOBJDIR)/trivial_decompression.o: $(GFXC)
	@echo "  GFXC    --decompression_suite --trivial"
	@$(MKDIR) -p $(@D)
	$(V)$(GFXC) --decompression_suite --trivial $@

$(TESTEXEDIR)/%.elf : $(TESTOBJDIR)/%.c.o $(TESTOBJDIR)/harness.c.o $(TESTOBJDIR)/benchmarks.c.o $(filter-out $(BUILDOBJDIR)/main.c.o $(BUILDOBJDIR)/scene/%.c.o $(BUILDOBJDIR)/management/keyinput.c.o, $(OBJS)) source/sys/gba_cart.ld
	@echo "  LD      $@"
	@$(MKDIR) -p $(@D)
	$(V)$(CC) -o $@ $(filter-out %.ld,$^) $(TESTLDFLAGS) -Wl,-Map,$(@:.elf=.map)

$(TESTREPORTDIR)/%.txt : $(TESTEXEDIR)/%.elf
	@echo "  MGBA    $<"
	@$(MKDIR) -p $(@D)
	$(V)bash -c "set -o pipefail; $(ROMTEST) -ClogLevel.gba.dma=16 -l15 -S0 -Rr0 $< | tee $@"

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

$(DMGNOTES): $(wildcard tools/dmg_notes/*.c) $(wildcard tools/dmg_notes/*.h) $(wildcard tools/dmg_notes/*.cpp)
	$(V)cd tools/dmg_notes && $(MAKE)

$(GBAFIX): $(wildcard tools/gbafix/*.c)
	$(V)cd tools/gbafix && $(MAKE)

$(GFXC): $(wildcard tools/gfxc/*.c) $(wildcard tools/gfxc/*.cpp) $(wildcard tools/gfxc/**/*.cpp)
	$(V)cd tools/gfxc && $(MAKE)

$(METADATA): $(wildcard tools/metadata/*.c) $(wildcard tools/metadata/*.h) $(wildcard tools/metadata/*.cpp) $(wildcard tools/gfxc/object.cpp) $(wildcard tools/gfxc/object.hpp)
	$(V)cd tools/metadata && $(MAKE)

generated_headers: $(BUILDSRCDIR)/dmg_music.h
$(BUILDSRCDIR)/dmg_music.h &: $(DMGNOTES)
	@echo "  DMG_NOTES --note-numbers"
	@$(MKDIR) -p $(BUILDSRCDIR)
	$(V)$(DMGNOTES) --note-numbers >$@

OBJS += $(BUILDOBJDIR)/dmg_music/frequencies.c.o
$(BUILDSRCDIR)/dmg_music/frequencies.c &: $(DMGNOTES)
	@echo "  DMG_NOTES --frequencies"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(DMGNOTES) --frequencies >$@

$(BUILDOBJDIR)/dmg_music/frequencies.c.o : $(BUILDSRCDIR)/dmg_music/frequencies.c | generated_headers
	@echo "  CC      $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

OBJS += $(BUILDOBJDIR)/dmg_music/staff_position.c.o
$(BUILDSRCDIR)/dmg_music/staff_position.c &: $(DMGNOTES)
	@echo "  DMG_NOTES --staff-position"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(DMGNOTES) --staff-position >$@

$(BUILDOBJDIR)/dmg_music/staff_position.c.o : $(BUILDSRCDIR)/dmg_music/staff_position.c | generated_headers
	@echo "  CC      $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<


generated_headers: $(BUILDSRCDIR)/graphics.h
OBJS += $(BUILDOBJDIR)/graphics.o
$(BUILDOBJDIR)/graphics.o $(BUILDSRCDIR)/graphics.h &: $(GFXC) $(SOURCES_PNG) $(SOURCES_TILEDMAP) $(SOURCES_TILEDSET)
	@echo "  GFXC"
	@$(MKDIR) -p $(BUILDOBJDIR)
	@$(MKDIR) -p $(BUILDSRCDIR)
	$(V)$(GFXC) $(GRAPHICSDIR) $(BUILDOBJDIR)/graphics.o $(BUILDSRCDIR)/graphics.h

generated_headers: $(BUILDSRCDIR)/graphics_types.h
$(BUILDSRCDIR)/graphics_types.h: $(GFXC)
	@echo "  GFXC    --structs"
	@$(MKDIR) -p $(BUILDSRCDIR)
	$(V)$(GFXC) --structs $(BUILDSRCDIR)/graphics_types.h

generated_headers: $(BUILDSRCDIR)/resource_credits.h
OBJS += $(BUILDOBJDIR)/resource_credits.o
$(BUILDOBJDIR)/resource_credits.o $(BUILDSRCDIR)/resource_credits.h &: $(METADATA) $(SOURCES_PNG)
	@echo "  METADATA"
	@$(MKDIR) -p $(BUILDSRCDIR)
	$(V)$(METADATA) \
		--out-object $(BUILDOBJDIR)/resource_credits.o \
		--out-header $(BUILDSRCDIR)/resource_credits.h \
		--out-text $(BUILDDIR)/main/CREDITS.md \
		$(SOURCES_PNG)

$(ELF): $(OBJS) source/sys/gba_cart.ld
	@echo "  LD      $@"
	$(V)$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(ROM): $(ELF) $(GBAFIX)
	@echo "  OBJCOPY $<"
	$(V)$(OBJCOPY) -O binary $< $@
	@echo "  GBAFIX  $@"
	$(V)$(GBAFIX) $@ -t$(GAME_TITLE) -c$(GAME_CODE)

$(HOSTEXEDIR)/test_vram_op_queue : $(HOSTOBJDIR_SRC)/management/vram_op_queue.c.o $(HOSTOBJDIR_SRC)/gba/palette.c.o $(HOSTOBJDIR_SRC)/gba/vram.c.o $(HOSTOBJDIR_SRC)/gba/oam.c.o $(HOSTOBJDIR_SRC)/gba/hw_reg.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_HOST)/graphics.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_SRC)/scene/walkaround.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_SRC)/gba/hw_reg.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_SRC)/gba/palette.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_SRC)/management/shadow_oam.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_SRC)/management/shadow_vram.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_SRC)/utils/ansi_text_palette.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_SRC)/utils/one_transparent_tileset.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_SRC)/mix_rgb.c.o
$(HOSTEXEDIR)/test_walkaround : $(HOSTOBJDIR_SRC)/text_printer.c.o
$(TESTEXEDIR)/bench_text_printer.elf : $(BUILDOBJDIR)/graphics.o
$(TESTEXEDIR)/bench_mode3_copy.elf : $(BUILDOBJDIR)/graphics.o
$(TESTEXEDIR)/bench_parallax_mountain.elf : $(BUILDOBJDIR)/graphics.o
$(TESTEXEDIR)/bench_parallax_mountain.elf : $(BUILDOBJDIR)/management/keyinput.c.o
$(TESTEXEDIR)/bench_parallax_mountain.elf : $(BUILDOBJDIR)/scene/parallax_mountain_dusk.c.o
$(TESTEXEDIR)/bench_decompress.elf : $(patsubst $(GRAPHICSDIR)/%.png,$(TESTOBJDIR)/%.png.o,$(SOURCES_PNG))
$(TESTEXEDIR)/bench_decompress.elf : $(TESTOBJDIR)/trivial_decompression.o
$(TESTEXEDIR)/bench_dmgMusicUsingNotation.elf : $(BUILDOBJDIR)/graphics.o
$(TESTEXEDIR)/bench_dmgMusicUsingNotation.elf : $(BUILDOBJDIR)/management/keyinput.c.o
$(TESTEXEDIR)/bench_dmgMusicUsingNotation.elf : $(BUILDOBJDIR)/scene/dmg_music_using_notation.c.o
$(TESTEXEDIR)/bench_dmgMusicUsingNotation.elf : $(BUILDOBJDIR)/dmg_music/frequencies.c.o
$(TESTEXEDIR)/bench_dmgMusicUsingNotation.elf : $(BUILDOBJDIR)/dmg_music/staff_position.c.o
$(TESTEXEDIR)/bench_walkaround.elf : $(BUILDOBJDIR)/graphics.o
$(TESTEXEDIR)/bench_walkaround.elf : $(BUILDOBJDIR)/management/keyinput.c.o
$(TESTEXEDIR)/bench_walkaround.elf : $(BUILDOBJDIR)/management/shadow_oam.c.o
$(TESTEXEDIR)/bench_walkaround.elf : $(BUILDOBJDIR)/management/shadow_vram.c.o
$(TESTEXEDIR)/bench_walkaround.elf : $(BUILDOBJDIR)/scene/walkaround.c.o
$(TESTEXEDIR)/test_walkaround.elf : $(BUILDOBJDIR)/graphics.o
$(TESTEXEDIR)/test_walkaround.elf : $(BUILDOBJDIR)/scene/walkaround.c.o

check_host: $(patsubst $(HOSTEXEDIR)/%,$(HOSTREPORTDIR)/%.txt,$(HOST_RUNNERS))
	@:

check_bench_decompress: build/test/exe/bench_decompress.elf
	$(V)$(ROMTEST) -ClogLevel.gba.dma=16 -l15 -S0 -Rr0 $<

check_mgba: $(patsubst $(TESTEXEDIR)/%.elf,$(TESTREPORTDIR)/%.txt,$(TEST_RUNNERS))
	@:

check_all: check_host check_mgba
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
	$(V)cd tools/metadata && $(MAKE) clean

generated_headers:
	@:

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)
-include user.mk

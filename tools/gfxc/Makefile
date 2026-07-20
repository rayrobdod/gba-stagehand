# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2022
# SPDX-FileContributor: Raymond Dodge, 2025-2026

# Program Configuration

NAME	:= gfxc

# Defines passed to all files

DEFINES	:=
LIBS	+= -lc
LIBS	+= -lpng
LIBS	+= -ltiled -lQt5Core -lQt5Gui

# Tools

CC	:= gcc
CXX	:= g++
RM	:= rm -rf
MKDIR	:= mkdir

# Verbose flag

V := @

# Directories

BUILDDIR	:= build
BUILDOBJDIR	:= build/objs
TEST_OBJ_DIR	:= build/test/objs
TEST_RUNNER_DIR	:= build/test/exe
TEST_REPORT_DIR	:= build/test/report

# Source files

SRCS_C	:= $(wildcard *.c)
SRCS_CPP	:= $(wildcard *.cpp) $(wildcard compression/*.cpp) $(wildcard resource_type/*.cpp)
SRCS_TEST_C	:= $(wildcard test/*.c)
SRCS_TEST_CPP	:= $(wildcard test/*.cpp)

# Flags

CFLAGS += -std=gnu11
CFLAGS += -I.
CFLAGS += -Wall -Wextra
CFLAGS += -Werror
CFLAGS += -ggdb -DDEBUG
CXXFLAGS += -std=gnu++20
CXXFLAGS += -I.
CXXFLAGS += -I/usr/include/x86_64-linux-gnu/qt5 -fPIC
CXXFLAGS += -I/usr/include/x86_64-linux-gnu/qt5/QtCore
CXXFLAGS += -I/usr/include/x86_64-linux-gnu/qt5/QtGui
CXXFLAGS += -Wall -Wextra
CXXFLAGS += -Werror
CXXFLAGS += -Wno-missing-field-initializers
CXXFLAGS += -ggdb -DDEBUG
CXXFLAGS += -fconcepts
LDFLAGS += $(LIBS)

# Intermediates

OBJS	:= \
	$(patsubst %.c,$(BUILDOBJDIR)/%.c.o,$(SRCS_C)) \
	$(patsubst %.cpp,$(BUILDOBJDIR)/%.cpp.o,$(SRCS_CPP)) \

TEST_OBJS := \
	$(patsubst test/%.c,$(TEST_OBJ_DIR)/%.c.o,$(SRCS_TEST_C)) \
	$(patsubst test/%.cpp,$(TEST_OBJ_DIR)/%.cpp.o,$(SRCS_TEST_CPP)) \

TEST_RUNNERS := \
	$(filter-out \
		$(TEST_RUNNER_DIR)/_shared, \
		$(patsubst test/%.c,$(TEST_RUNNER_DIR)/%,$(SRCS_TEST_C)) \
		$(patsubst test/%.cpp,$(TEST_RUNNER_DIR)/%,$(SRCS_TEST_CPP)) \
	)

TEST_REPORTS := \
	$(patsubst $(TEST_RUNNER_DIR)/%,$(TEST_REPORT_DIR)/%.txt,$(TEST_RUNNERS))

DEPS	:= $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# Rules

$(BUILDOBJDIR)/%.c.o : %.c
	@echo "  HOSTCC  $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILDOBJDIR)/%.cpp.o : %.cpp
	@echo "  HOSTCXX $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(TEST_OBJ_DIR)/%.cpp.o : test/%.cpp
	@echo "  HOSTCXX $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(TEST_RUNNER_DIR)/% : $(TEST_OBJ_DIR)/%.cpp.o $(TEST_OBJ_DIR)/_shared.cpp.o
	@echo "  HOSTLD  $@"
	@$(MKDIR) -p $(@D)
	$(V)$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_REPORT_DIR)/%.txt : $(TEST_RUNNER_DIR)/%
	@echo "  CALL    $<"
	@$(MKDIR) -p $(@D)
	$(V)bash -c "set -o pipefail; $< | tee $@"

# Targets

# Clear the default suffixes
.SUFFIXES:
# Don't delete intermediate files
.SECONDARY:
# Delete files that weren't built properly
.DELETE_ON_ERROR:
# Secondary expansion is required for dependency variables in object rules.
.SECONDEXPANSION:

.PHONY: all check clean

all: $(NAME)
all: standalone_smol

$(NAME): $(filter-out $(BUILDOBJDIR)/standalone_smol.cpp.o, $(OBJS))
	@echo "  HOSTLD  $@"
	$(V)$(CXX) -o $@ $^ $(LDFLAGS)

standalone_smol: $(BUILDOBJDIR)/standalone_smol.cpp.o $(BUILDOBJDIR)/compression/smol.cpp.o $(BUILDOBJDIR)/ints.cpp.o $(BUILDOBJDIR)/compare_mismatch_error_message.cpp.o
	@echo "  HOSTLD  $@"
	$(V)$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_RUNNER_DIR)/test_huff: $(BUILDOBJDIR)/ints.cpp.o
$(TEST_RUNNER_DIR)/test_subword_output_iterator: $(BUILDOBJDIR)/ints.cpp.o
$(TEST_RUNNER_DIR)/test_lz: $(BUILDOBJDIR)/ints.cpp.o $(BUILDOBJDIR)/compression/lz.cpp.o
$(TEST_RUNNER_DIR)/test_frit: $(BUILDOBJDIR)/ints.cpp.o $(BUILDOBJDIR)/compression/frit.cpp.o
$(TEST_RUNNER_DIR)/test_smol: $(BUILDOBJDIR)/ints.cpp.o $(BUILDOBJDIR)/compression/smol.cpp.o

check: $(TEST_REPORTS)
	@:

clean:
	@echo "  CLEAN"
	$(V)$(RM) $(NAME)
	$(V)$(RM) standalone_smol
	$(V)$(RM) $(BUILDDIR)

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)

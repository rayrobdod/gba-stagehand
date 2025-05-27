# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2022
# SPDX-FileContributor: Raymond Dodge, 2025

# Program Configuration

NAME	:= gfxc

# Defines passed to all files

DEFINES	:=
LIBS	:= -lc -lpng

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

# Source files

SRCS_C	:= $(wildcard *.c)
SRCS_CPP	:= $(wildcard *.cpp) $(wildcard compression/*.cpp) $(wildcard resource_type/*.cpp)
SRCS_TEST_C	:= $(wildcard test/*.c)
SRCS_TEST_CPP	:= $(wildcard test/*.cpp)

# Flags

CFLAGS += -std=gnu11
CFLAGS += -I../../source
CFLAGS += -I../gfx2obj
CFLAGS += -I.
CFLAGS += -Wall
CFLAGS += -ggdb -DDEBUG
CXXFLAGS += -std=gnu++20
CXXFLAGS += -I../../source
CXXFLAGS += -I../gfx2obj
CXXFLAGS += -I.
CXXFLAGS += -Wall
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

$(NAME): $(OBJS)
	@echo "  HOSTLD  $@"
	$(V)$(CXX) -o $@ $(OBJS) $(LDFLAGS)

$(TEST_RUNNER_DIR)/test_huff: $(BUILDOBJDIR)/ints.cpp.o
$(TEST_RUNNER_DIR)/test_subword_output_iterator: $(BUILDOBJDIR)/ints.cpp.o
$(TEST_RUNNER_DIR)/test_lz: $(BUILDOBJDIR)/ints.cpp.o $(BUILDOBJDIR)/compression/lz.cpp.o

check: $(TEST_RUNNERS)
	$(V)for r in $(TEST_RUNNERS); do $$r ; done

clean:
	@echo "  CLEAN"
	$(V)$(RM) $(NAME)
	$(V)$(RM) $(BUILDDIR)

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)

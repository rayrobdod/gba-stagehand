# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2022
# SPDX-FileContributor: Raymond Dodge, 2025

# Program Configuration

NAME	:= dmg_notes

# Defines passed to all files

DEFINES	:=
LIBS	:= -lm

# Tools

CC	:= gcc
CXX	:= g++
RM	:= rm -rf
MKDIR	:= mkdir

# Verbose flag

V := @

# Directories

BUILDDIR	:= build
BUILDOBJDIR	:= $(BUILDDIR)/objs

# Source files

SRCS_C	:= $(wildcard *.c)
SRCS_CPP	:= $(wildcard *.cpp)

# Flags

CFLAGS += -std=gnu11
CFLAGS += -I.
CFLAGS += -Wall -Wextra
CFLAGS += -Werror
CFLAGS += -ggdb -DDEBUG
CXXFLAGS += -std=gnu++20
CXXFLAGS += -I.
CXXFLAGS += -I../gfxc
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

DEPS	:= $(OBJS:.o=.d)

# Rules

$(BUILDOBJDIR)/%.c.o : %.c
	@echo "  HOSTCC  $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILDOBJDIR)/%.cpp.o : %.cpp
	@echo "  HOSTCXX $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

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

clean:
	@echo "  CLEAN"
	$(V)$(RM) $(NAME)
	$(V)$(RM) $(BUILDDIR)

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)

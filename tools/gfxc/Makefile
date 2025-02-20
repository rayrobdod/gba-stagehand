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

# Source files

SRCS_C	:= $(wildcard *.c)
SRCS_CPP	:= $(wildcard *.cpp)

# Flags

CFLAGS += -I../../source
CFLAGS += -I../gfx2obj
CFLAGS += -Wall
CFLAGS += -ggdb -DDEBUG
CXXFLAGS += -I../../source
CXXFLAGS += -I../gfx2obj
CXXFLAGS += -Wall
CXXFLAGS += -ggdb -DDEBUG
LDFLAGS += $(LIBS)

# Intermediates

OBJS	:= \
	$(BUILDDIR)/object.cpp.o \
	$(patsubst %.c,$(BUILDDIR)/%.c.o,$(SRCS_C)) \
	$(patsubst %.cpp,$(BUILDDIR)/%.cpp.o,$(SRCS_CPP)) \

DEPS	:= $(OBJS:.o=.d)

# Rules

$(BUILDDIR)/%.c.o : %.c
	@echo "  HOSTCC  $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILDDIR)/%.cpp.o : %.cpp
	@echo "  HOSTCXX $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

# Targets

.PHONY: all clean

all: $(NAME)

$(BUILDDIR)/object.cpp.o : ../gfx2obj/object.cpp
	@echo "  HOSTCXX $<"
	@$(MKDIR) -p $(@D)
	$(V)$(CXX) $(CFLAGS) -MMD -MP -c -o $@ $<

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

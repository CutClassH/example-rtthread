# Copyright 2019 SiFive, Inc #
# SPDX-License-Identifier: Apache-2.0 #

# Provide a default for no verbose output
HIDE ?= @

PROGRAM ?= example-rtthread

OBJ_DIR ?= ./$(CONFIGURATION)/build

C_SOURCES = $(wildcard *.c)

PWD_PATH :=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# ----------------------------------------------------------------------
# RT-Thread 
# ----------------------------------------------------------------------
RTTHREAD_SOURCE_PATH ?= ../../rtthread-metal
#     Include FREERTOS source
include $(RTTHREAD_SOURCE_PATH)/rtthread.mk

#     Add RT-Thread include 
_COMMON_CFLAGS  += -I$(PWD_PATH)
_COMMON_CFLAGS  += ${RTTHREAD_INC}

CFLAGS += -Wl,--keep-symbol finsh_system_init

#     Update our list of C source files.
C_SOURCES += ${RTTHREAD_SRC_C}

#     Update our list of S source files.
S_SOURCES += ${RTTHREAD_SRC_S}

# ----------------------------------------------------------------------
# Build List of Object according C/CPP/S source file list
# ----------------------------------------------------------------------
_C_OBJ_FILES   += $(C_SOURCES:%.c=${OBJ_DIR}/%.o)
_CXX_OBJ_FILES += $(CXX_SOURCES:%.cpp=${OBJ_DIR}/%.o)

_asm_s := $(filter %.s,$(S_SOURCES))
_asm_S := $(filter %.S,$(S_SOURCES))
_ASM_OBJ_FILES := $(_asm_s:%.s=${OBJ_DIR}/%.o) $(_asm_S:%.S=${OBJ_DIR}/%.o)

OBJS += ${_C_OBJ_FILES}
OBJS += ${_CXX_OBJ_FILES}
OBJS += ${_ASM_OBJ_FILES}

# ----------------------------------------------------------------------
# Compile Object Files From Assembly
# ----------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.S
	@echo "Assemble: $<"
	$(HIDE)$(CC) -D__ASSEMBLY__ -c -o $@ $(ASFLAGS) $(CPPFLAGS) $(_COMMON_CFLAGS) $<
	@echo

# ----------------------------------------------------------------------
# Compile Object Files From C
# ----------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c
	@echo "Compile: $<"
	$(HIDE)$(CC) -c -o $@ $(CFLAGS) $(CPPFLAGS) $(CFLAGS_COMMON) $(_COMMON_CFLAGS) $<
	@echo

# ----------------------------------------------------------------------
# Compile Object Files From CPP
# ----------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.cpp
	@echo "Compile: $<"
	$(HIDE)$(CXX) -c -o $@ $(CXXFLAGS) $(CPPFLAGS) $(CFLAGS_COMMON) $(_COMMON_CFLAGS) $<

# ----------------------------------------------------------------------
# Add custom flags for link
# ----------------------------------------------------------------------
# Reduce default size of the stack and the heap
#
# _ADD_LDFLAGS  += -Wl,--defsym,__stack_size=0x200
# _ADD_LDFLAGS  += -Wl,--defsym,__heap_size=0x4D0

# ----------------------------------------------------------------------
# create dedicated directory for Object files
# ----------------------------------------------------------------------
BUILD_DIRECTORIES = \
        $(OBJ_DIR) 

# ----------------------------------------------------------------------
# Build rules
# ----------------------------------------------------------------------
$(BUILD_DIRECTORIES):
	mkdir -p $@

directories: $(BUILD_DIRECTORIES)

$(PROGRAM): \
	directories \
	$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(_ADD_LDFLAGS) $(OBJS) $(LOADLIBES) $(LDLIBS) -o $@
	@echo

clean::
	rm -rf $(BUILD_DIRECTORIES)
	rm -f $(PROGRAM) $(PROGRAM).hex



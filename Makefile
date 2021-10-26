################################################################################
#
#	Makefile for Mk
#	You could also just build Mk with:
#
#		cc -o mk mk*.c
#
################################################################################

INSTALLDIR ?= /usr/local
INSTALLBINDIR ?= $(INSTALLDIR)/bin
INSTALLMANDIR ?= $(INSTALLDIR)/share/man

VALID_CONFIGS := DEBUG RELEASE
CONFIG        ?= DEBUG

ifeq ($(filter $(VALID_CONFIGS),$(CONFIG)),)
 $(error CONFIG must be one of: $(VALID_CONFIG))
endif

VERSION_RELEASE := $(shell date +'%Y%m%d')
VERSION_DEBUG   := $(shell date +'%Y%m%d%H%M%S')

VERSION := $(VERSION_$(CONFIG))

ifeq ($(patsubst MINGW%,MINGW,$(shell uname -s)),MINGW)
 PLATNAME := WIN32
else
 ifeq ($(shell uname), Darwin)
  PLATNAME := MACOS
 else
  PLATNAME := LINUX
 endif
endif

PLATFORM_WIN32 := mswin
PLATFORM_MACOS := macos
PLATFORM_LINUX := linux

PLATFORM := $(PLATFORM_$(PLATNAME))
NOTPLATFORM := $(filter-out $(PLATFORM),$(PLATFORM_WIN32) $(PLATFORM_MACOS) $(PLATFORM_LINUX))

PLATBUILD_WIN32 := Win32
PLATBUILD_MACOS := macOS
PLATBUILD_LINUX := Linux

PLATBUILD := $(PLATBUILD_$(PLATNAME))

SUFFIX_WIN32 := .exe
SUFFIX_MACOS :=
SUFFIX_LINUX :=

SUFFIX := $(SUFFIX_$(PLATNAME))

VALID_TOOLCHAINS := CLANG GCC
TOOLCHAIN ?= CLANG

# NOTE: Clang 3.1 complains about failing to export symbols due to them not being found... [win32]

ifeq ($(filter $(VALID_TOOLCHAINS),$(TOOLCHAIN)),)
 $(error TOOLCHAIN must be one of: $(VALID_TOOLCHAINS))
endif

NAME_CLANG := Clang
LINK_CLANG := clang
LIBS_CLANG :=
CC_CLANG   := clang

NAME_GCC := GCC
LINK_GCC := gcc
LIBS_GCC :=
CC_GCC   := gcc

CXX_NAME := $(NAME_$(TOOLCHAIN))
LINK     := $(LINK_$(TOOLCHAIN))
LIBS     := $(LIBS_$(TOOLCHAIN))
CC       := $(CC_$(TOOLCHAIN))

ROOT_DIR  ?=
BUILD_DIR ?= $(ROOT_DIR)

PROJ_DIR  ?= $(ROOT_DIR)

PROJ_SRCDIR ?= $(PROJ_DIR)src/
PROJ_INCDIR ?= 

INCDIRS := $(PROJ_INCDIR)

INCFLAGS_CLANG = $(foreach I,$(INCDIRS),"-I$(I)")
INCFLAGS_GCC   = $(foreach I,$(INCDIRS),"-I$(I)")

INCFLAGS := $(INCFLAGS_$(TOOLCHAIN))

GCC_CLANG_WARNINGS := -Wall -Wextra -Warray-bounds -pedantic -Wno-return-type-c-linkage
GCC_CLANG_FATALERR := -Wfatal-errors
GCC_CLANG_STDCFLAG := -std=c11
GCC_CLANG_PLATFORM := -msse2 -march=core2

GCC_CLANG_OPTTYPES := WARNINGS FATALERR PLATFORM

GCC_CLANG_COMMON := $(foreach X,$(GCC_CLANG_OPTTYPES),$(GCC_CLANG_$(X)))

CFLAGS_ALL_GCC   := $(GCC_CLANG_COMMON)
CFLAGS_ALL_CLANG := $(GCC_CLANG_COMMON)

CFLAGS_ALL := $(CFLAGS_ALL_$(TOOLCHAIN)) $(INCFLAGS)

CFLAGS_STDC   := $($(TOOLCHAIN)_STDCFLAG)

GCC_CLANG_DEBUG   := -g -D_DEBUG
GCC_CLANG_RELEASE := -DNDEBUG -O2 -fomit-frame-pointer -fno-strict-aliasing

CFLAGS_DEBUG_GCC   := $(GCC_CLANG_DEBUG)
CFLAGS_DEBUG_CLANG := $(GCC_CLANG_DEBUG)

CFLAGS_DEBUG := $(CFLAGS_DEBUG_$(TOOLCHAIN))

CFLAGS_RELEASE_GCC   := $(GCC_CLANG_RELEASE)
CFLAGS_RELEASE_CLANG := $(GCC_CLANG_RELEASE)

CFLAGS_RELEASE := $(CFLAGS_RELEASE_$(TOOLCHAIN))

CFLAGS_D := $(CFLAGS_ALL) $(CFLAGS_DEBUG)
CFLAGS_R := $(CFLAGS_ALL) $(CFLAGS_RELEASE)
CFLAGS   := $(CFLAGS_ALL) $(CFLAGS_$(CONFIG))

DEPGEN_GCC   := -MD -MP
DEPGEN_CLANG := -MD -MP
DEPGEN       := $(DEPGEN_$(TOOLCHAIN))

COMPILEARG_GCC   := -c
COMPILEARG_CLANG := -c
COMPILEARG       := $(COMPILEARG_$(TOOLCHAIN))

OUTARG_GCC   := -o
OUTARG_CLANG := -o
OUTARG       := $(OUTARG_$(TOOLCHAIN))

LDOUTARG_GCC   := -o
LDOUTARG_CLANG := -o
LDOUTARG       := $(LDOUTARG_$(TOOLCHAIN))

SHARED_GCC   := -shared
SHARED_CLANG := -shared
SHARED       := $(SHARED_$(TOOLCHAIN))

LFLAGS_GCC_CLANG_WIN32 := -static -pthread -static-libgcc
# -Wl,-Bstatic -lpthread -Wl,-Bstatic -lwinpthread

LFLAGS_GCC_CLANG_COMMON := $(LFLAGS_GCC_CLANG_$(PLATNAME))

LFLAGS_D_GCC   := $(LFLAGS_GCC_CLANG_COMMON)
LFLAGS_D_CLANG := $(filter-out -pthread,$(LFLAGS_GCC_CLANG_COMMON))
LFLAGS_D       := $(LFLAGS_D_$(TOOLCHAIN))

LFLAGS_R_GCC   := $(LFLAGS_GCC_CLANG_COMMON)
LFLAGS_R_CLANG := $(filter-out -pthread,$(LFLAGS_GCC_CLANG_COMMON))
LFLAGS_R       := $(LFLAGS_R_$(TOOLCHAIN))

DEFARG_GCC   := -D
DEFARG_CLANG := -D
DEFARG       := $(DEFARG_$(TOOLCHAIN))

C_EXT   := .c
H_EXT   := .h
OBJ_EXT := .o
DEP_EXT := .d

SRC_EXTS := $(C_EXT) $(CPP_EXT)
HDR_EXTS := $(H_EXT) $(HPP_EXT)

EXE_SRCDIR := $(PROJ_SRCDIR)

INTDIR := $(BUILD_DIR).make/$(PLATFORM)/

OBJDIR   := $(INTDIR)obj/$(CXX_NAME)/
OBJDIR_D := $(OBJDIR)debug/
OBJDIR_R := $(OBJDIR)release/

BINDIR := $(BUILD_DIR)bin/
LIBDIR := $(BUILD_DIR)lib/$(CXX_NAME)/

EXENAME         := mk
EXEFILE_DEBUG   := $(EXENAME)-debug
EXEFILE_RELEASE := $(EXENAME)
EXEFILE         := $(EXEFILE_$(CONFIG))

EXE_C_SOURCES   := $(shell find "$(patsubst %/,%,$(EXE_SRCDIR))" -type f -name "*.c" $(foreach N,$(NOTPLATFORM),\! -path \*/$(N)/\*))
EXE_C_OBJECTS_D := $(patsubst $(EXE_SRCDIR)%,$(OBJDIR_D)%$(OBJ_EXT),$(EXE_C_SOURCES))
EXE_C_OBJECTS_R := $(patsubst $(EXE_SRCDIR)%,$(OBJDIR_R)%$(OBJ_EXT),$(EXE_C_SOURCES))
EXE_OBJECTS_D   := $(EXE_C_OBJECTS_D) $(EXE_CXX_OBJECTS_D)
EXE_OBJECTS_R   := $(EXE_C_OBJECTS_R) $(EXE_CXX_OBJECTS_R)
EXE_DEPENDS_D   := $(patsubst %$(OBJ_EXT),%$(DEP_EXT),$(EXE_OBJECTS_D))
EXE_DEPENDS_R   := $(patsubst %$(OBJ_EXT),%$(DEP_EXT),$(EXE_OBJECTS_R))
EXE_TARGET_D    := $(BINDIR)$(EXEFILE_DEBUG)$(SUFFIX)
EXE_TARGET_R    := $(BINDIR)$(EXEFILE_RELEASE)$(SUFFIX)

EXE_TARGET_DEBUG   := $(EXE_TARGET_D)
EXE_TARGET_RELEASE := $(EXE_TARGET_R)
EXE_TARGET         := $(EXE_TARGET_$(CONFIG))

EXE_OBJECTS := $(EXE_OBJECTS_D) $(EXE_OBJECTS_R)
EXE_DEPENDS := $(EXE_DEPENDS_D) $(EXE_DEPENDS_R)
EXE_TARGETS := $(EXE_TARGET_D) $(EXE_TARGET_R)

ALL_PROJECTS := EXE

ALL_OBJECTS := $(foreach X,$(ALL_PROJECTS),$($(X)_OBJECTS))
ALL_DEPENDS := $(foreach X,$(ALL_PROJECTS),$($(X)_DEPENDS))
ALL_TARGETS := $(foreach X,$(ALL_PROJECTS),$($(X)_TARGETS))

CLEANFILES := $(ALL_OBJECTS) $(ALL_DEPENDS) $(ALL_TARGETS)




.PHONY: all debug release install clean
.IGNORE: clean




all: $(ALL_TARGETS)

debug: $(EXE_TARGET_D)

release: $(EXE_TARGET_R)

clean: 
	-@rm -rf $(OBJDIR)* $(wildcard $(CLEANFILES)) 2>/dev/null

install: release
	@mkdir -p "$(INSTALLBINDIR)"
	@mkdir -p "$(INSTALLMANDIR)/man1"
	@install -v -C    -m 644 doc/mk.1 "$(INSTALLMANDIR)/man1"
	@install -v -C -s -m 551 "$(EXE_TARGET_R)" "$(INSTALLBINDIR)"

install-debug: install debug
	@install -v -C    -m 551 "$(EXE_TARGET_D)" "$(INSTALLBINDIR)"




$(EXE_C_OBJECTS_D): $(OBJDIR_D)%$(C_EXT)$(OBJ_EXT): $(EXE_SRCDIR)%$(C_EXT) Makefile
	@mkdir -p $(dir $@)
	$(info >$<)
	@$(CC) $(CFLAGS_STDC) $(CFLAGS_D) $(DEPGEN) $(OUTARG) "$@" $(COMPILEARG) "$<"

$(EXE_C_OBJECTS_R): $(OBJDIR_R)%$(C_EXT)$(OBJ_EXT): $(EXE_SRCDIR)%$(C_EXT) Makefile
	@mkdir -p $(dir $@)
	$(info >$<)
	@$(CC) $(CFLAGS_STDC) $(CFLAGS_R) $(DEPGEN) $(OUTARG) "$@" $(COMPILEARG) "$<"




$(EXE_TARGET_D): $(EXE_OBJECTS_D)
	@mkdir -p $(dir $@)
	$(info <$@)
	@$(LINK) $(LDOUTARG) "$@" $+ $(LFLAGS_D)

$(EXE_TARGET_R): $(EXE_OBJECTS_R)
	@mkdir -p $(dir $@)
	$(info <$@)
	@$(LINK) $(LDOUTARG) "$@" $+ $(LFLAGS_R)




ifneq ($(MAKECMDGOALS),clean)
 -include $(EXE_DEPENDS_D) $(EXE_DEPENDS_R)
endif

## Makefile
## Summary: Builds the ppn CLI against precompiled static kclib artifacts.
##
## Author:  KaisarCode
## Website: https://kaisarcode.com
## License: GNU LGPL v3

SHELL := /bin/sh

ifndef KCLIB_DIR
$(error KCLIB_DIR environment variable is required)
endif
PPN_PROJECT_DIR := $(KCLIB_DIR)/proj/libppn.c
PPN_BIN_DIR := $(PPN_PROJECT_DIR)/bin
PPN_INCLUDE_DIR := $(PPN_PROJECT_DIR)/src
BIN_DIR := bin
BUILD_DIR := .build

XDG_DATA_HOME ?= $(HOME)/.local/share
OSXCROSS_ROOT ?= $(XDG_DATA_HOME)/osxcross/target
OSXCROSS_X86_64_CC := $(OSXCROSS_ROOT)/bin/o64-clang
OSXCROSS_AARCH64_CC := $(OSXCROSS_ROOT)/bin/oa64-clang
MACOSX_OTOOL := $(firstword $(wildcard $(OSXCROSS_ROOT)/bin/*-apple-darwin*-otool))
MACOSX_DEPLOYMENT_TARGET ?=
WINE ?= wine
WINE_X86_64_CC ?= x86_64-w64-mingw32-gcc
CC ?= cc

HOST_ARCH := $(shell uname -m)
HOST_SYSTEM := $(shell uname -s)
NATIVE_ARCH := unsupported
NATIVE_PLATFORM := unsupported

ifneq ($(filter x86_64 amd64,$(HOST_ARCH)),)
NATIVE_ARCH := x86_64
endif
ifeq ($(HOST_SYSTEM),Linux)
NATIVE_PLATFORM := linux
endif
ifeq ($(HOST_SYSTEM),Darwin)
NATIVE_PLATFORM := macos
endif
ifneq ($(filter MINGW% MSYS% CYGWIN%,$(HOST_SYSTEM)),)
NATIVE_PLATFORM := windows
endif

NATIVE_TARGET := $(NATIVE_ARCH)/$(NATIVE_PLATFORM)
NATIVE_EXE_EXT :=
ifeq ($(NATIVE_PLATFORM),windows)
NATIVE_EXE_EXT := .exe
endif

.DEFAULT_GOAL := $(NATIVE_TARGET)

.PHONY: all test wine clean \
	x86_64/linux x86_64/windows x86_64/macos aarch64/macos

all: x86_64/linux x86_64/windows x86_64/macos aarch64/macos

define linux_target
	@target='$(PPN_BIN_DIR)/$(1)/linux'; \
	header='$(PPN_INCLUDE_DIR)/libppn.h'; \
	library="$$target/libppn.a"; \
	out='$(BIN_DIR)/$(1)/linux/ppn'; \
	[ -f "$$header" ] || { echo "kccli: missing libppn header: $$header" >&2; exit 1; }; \
	[ -f "$$library" ] || { echo "kccli: missing libppn static library: $$library" >&2; exit 1; }; \
	command -v $(2) >/dev/null 2>&1 || { echo "kccli: missing compiler: $(2)" >&2; exit 1; }; \
	mkdir -p '$(BIN_DIR)/$(1)/linux'; \
	$(2) -std=c11 -O2 -Wall -Wextra -Werror -I'$(PPN_INCLUDE_DIR)' src/ppn.c \
		"$$library" -o "$$out" || exit 1; \
	echo "OK $(1)/linux"
endef

x86_64/linux:
	$(call linux_target,x86_64,gcc)

define windows_target
	@target='$(PPN_BIN_DIR)/$(1)/windows'; \
	header='$(PPN_INCLUDE_DIR)/libppn.h'; \
	library="$$target/libppn.a"; \
	out='$(BIN_DIR)/$(1)/windows/ppn.exe'; \
	[ -f "$$header" ] || { echo "kccli: missing libppn header: $$header" >&2; exit 1; }; \
	[ -f "$$library" ] || { echo "kccli: missing libppn static library: $$library" >&2; exit 1; }; \
	command -v $(2) >/dev/null 2>&1 || { echo "kccli: missing compiler: $(2)" >&2; exit 1; }; \
	mkdir -p '$(BIN_DIR)/$(1)/windows'; \
	$(2) -std=c11 -O2 -Wall -Wextra -Werror -I'$(PPN_INCLUDE_DIR)' src/ppn.c \
		"$$library" -luser32 -lshell32 -o "$$out" || exit 1; \
	echo "OK $(1)/windows"
endef

x86_64/windows:
	$(call windows_target,x86_64,x86_64-w64-mingw32-gcc)

define macos_target
	@target='$(PPN_BIN_DIR)/$(1)/macos'; \
	header='$(PPN_INCLUDE_DIR)/libppn.h'; \
	library="$$target/libppn.a"; \
	out='$(BIN_DIR)/$(1)/macos/ppn'; \
	[ -f "$$header" ] || { echo "kccli: missing libppn header: $$header" >&2; exit 1; }; \
	[ -f "$$library" ] || { echo "kccli: missing libppn static library: $$library" >&2; exit 1; }; \
	[ -x '$(2)' ] || { echo "kccli: missing macOS cross-compiler: $(2)" >&2; exit 1; }; \
	mkdir -p '$(BIN_DIR)/$(1)/macos'; \
	min='$(MACOSX_DEPLOYMENT_TARGET)'; \
	if [ -z "$$min" ]; then \
		[ -x '$(MACOSX_OTOOL)' ] || { echo "kccli: missing macOS otool: set MACOSX_DEPLOYMENT_TARGET" >&2; exit 1; }; \
		min=$$('$(MACOSX_OTOOL)' -l "$$library" 2>/dev/null | awk '/LC_BUILD_VERSION|LC_VERSION_MIN_MACOSX/ { f = 1; next } f && /minos|version/ { v = $$2 + 0; if (v > m) { m = v; s = $$2 }; f = 0 } END { if (m > 0) print s }'); \
	fi; \
	[ -n "$$min" ] || { echo "kccli: cannot determine macOS deployment target from $$library" >&2; exit 1; }; \
	LD_LIBRARY_PATH='$(OSXCROSS_ROOT)/lib':$$LD_LIBRARY_PATH \
	PATH='$(OSXCROSS_ROOT)/bin':$$PATH \
	$(2) -std=c11 -O2 -Wall -Wextra -Werror \
		-mmacosx-version-min="$$min" -I'$(PPN_INCLUDE_DIR)' src/ppn.c "$$library" \
		-framework Foundation -o "$$out" || exit 1; \
	echo "OK $(1)/macos"
endef

x86_64/macos:
	$(call macos_target,x86_64,$(OSXCROSS_X86_64_CC))

aarch64/macos:
	$(call macos_target,aarch64,$(OSXCROSS_AARCH64_CC))

test:
	@if [ -n "$(filter wine,$(MAKECMDGOALS))" ]; then \
		$(MAKE) --no-print-directory x86_64/windows; \
		command -v $(WINE) >/dev/null 2>&1 || { echo "kccli: missing Wine runtime: $(WINE)" >&2; exit 1; }; \
		command -v $(WINE_X86_64_CC) >/dev/null 2>&1 || { echo "kccli: missing Windows compiler: $(WINE_X86_64_CC)" >&2; exit 1; }; \
		mkdir -p '$(BUILD_DIR)/test-wine'; \
		cp '$(BIN_DIR)/x86_64/windows/ppn.exe' '$(BUILD_DIR)/test-wine/ppn.exe'; \
		$(WINE_X86_64_CC) -std=c11 -O2 -Wall -Wextra -Werror \
			-DKC_CLI_PATH=\"ppn.exe\" src/test.c \
			-o '$(BUILD_DIR)/test-wine/kccli_test.exe'; \
		cd '$(BUILD_DIR)/test-wine' && \
			WINEDEBUG=-all $(WINE) kccli_test.exe; \
	else \
		if [ "$(NATIVE_ARCH)" = "unsupported" ] || [ "$(NATIVE_PLATFORM)" = "unsupported" ]; then \
			echo "kccli: unsupported native target $(HOST_ARCH)/$(HOST_SYSTEM)" >&2; \
			exit 1; \
		fi; \
		$(MAKE) --no-print-directory '$(NATIVE_TARGET)'; \
		mkdir -p '$(BUILD_DIR)/test'; \
		$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
			-DKC_CLI_PATH=\"$(CURDIR)/$(BIN_DIR)/$(NATIVE_TARGET)/ppn$(NATIVE_EXE_EXT)\" \
			src/test.c -o '$(BUILD_DIR)/test/kccli_test'; \
		'$(BUILD_DIR)/test/kccli_test'; \
	fi

wine:
	@if [ -z "$(filter test,$(MAKECMDGOALS))" ]; then \
		echo "Use 'make test wine' to run Windows CLI tests through Wine." >&2; \
		exit 1; \
	fi
	@:

clean:
	@rm -rf '$(BUILD_DIR)' '$(BIN_DIR)'
	@echo "OK clean"

## Makefile
## Summary: Builds the menu CLI against precompiled static kclib artifacts.
##
## Author:  KaisarCode
## Website: https://kaisarcode.com
## License: GNU GPL v3

SHELL := /bin/sh

KCLIB_DIST_DIR ?= $(abspath $(CURDIR)/../../../kclib/dist)
MENU_DIST_DIR := $(KCLIB_DIST_DIR)/libmenu.c
BIN_DIR := bin
BUILD_DIR := .build

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
ifneq ($(filter i386 i686,$(HOST_ARCH)),)
NATIVE_ARCH := i686
endif
ifneq ($(filter aarch64 arm64,$(HOST_ARCH)),)
NATIVE_ARCH := aarch64
endif
ifneq ($(filter armv7l armv7%,$(HOST_ARCH)),)
NATIVE_ARCH := armv7
endif
ifneq ($(filter ppc64le powerpc64le,$(HOST_ARCH)),)
NATIVE_ARCH := powerpc64le
endif
ifneq ($(filter riscv64 s390x loongarch64 mips64el mipsel mips,$(HOST_ARCH)),)
NATIVE_ARCH := $(HOST_ARCH)
endif

ifeq ($(HOST_SYSTEM),Linux)
NATIVE_PLATFORM := linux
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
	x86_64/linux x86_64/windows \
	i686/linux i686/windows \
	aarch64/linux armv7/linux armv7hf/linux \
	riscv64/linux powerpc64le/linux \
	mips/linux mipsel/linux mips64el/linux \
	s390x/linux loongarch64/linux

all: \
	x86_64/linux x86_64/windows \
	i686/linux i686/windows \
	aarch64/linux armv7/linux armv7hf/linux \
	riscv64/linux powerpc64le/linux \
	mips/linux mipsel/linux mips64el/linux \
	s390x/linux loongarch64/linux

define linux_target
	@target='$(MENU_DIST_DIR)/$(1)/linux'; \
	header="$$target/libmenu.h"; \
	library="$$target/libmenu.a"; \
	out='$(BIN_DIR)/$(1)/linux/menu'; \
	[ -f "$$header" ] || { echo "kccli: missing libmenu header: $$header" >&2; exit 1; }; \
	[ -f "$$library" ] || { echo "kccli: missing libmenu static library: $$library" >&2; exit 1; }; \
	command -v $(2) >/dev/null 2>&1 || { echo "kccli: missing compiler: $(2)" >&2; exit 1; }; \
	mkdir -p '$(BIN_DIR)/$(1)/linux'; \
	$(2) -std=c11 -O2 -Wall -Wextra -Werror -I"$$target" src/menu.c "$$library" -o "$$out"; \
	echo "OK $(1)/linux"
endef

x86_64/linux:
	$(call linux_target,x86_64,gcc)

i686/linux:
	$(call linux_target,i686,i686-linux-gnu-gcc)

aarch64/linux:
	$(call linux_target,aarch64,aarch64-linux-gnu-gcc)

armv7/linux:
	$(call linux_target,armv7,arm-linux-gnueabi-gcc)

armv7hf/linux:
	$(call linux_target,armv7hf,arm-linux-gnueabihf-gcc)

riscv64/linux:
	$(call linux_target,riscv64,riscv64-linux-gnu-gcc)

powerpc64le/linux:
	$(call linux_target,powerpc64le,powerpc64le-linux-gnu-gcc)

mips/linux:
	$(call linux_target,mips,mips-linux-gnu-gcc)

mipsel/linux:
	$(call linux_target,mipsel,mipsel-linux-gnu-gcc)

mips64el/linux:
	$(call linux_target,mips64el,mips64el-linux-gnuabi64-gcc)

s390x/linux:
	$(call linux_target,s390x,s390x-linux-gnu-gcc)

loongarch64/linux:
	$(call linux_target,loongarch64,loongarch64-linux-gnu-gcc)

define windows_target
	@target='$(MENU_DIST_DIR)/$(1)/windows'; \
	header="$$target/libmenu.h"; \
	library="$$target/libmenu.a"; \
	out='$(BIN_DIR)/$(1)/windows/menu.exe'; \
	[ -f "$$header" ] || { echo "kccli: missing libmenu header: $$header" >&2; exit 1; }; \
	[ -f "$$library" ] || { echo "kccli: missing libmenu static library: $$library" >&2; exit 1; }; \
	command -v $(2) >/dev/null 2>&1 || { echo "kccli: missing compiler: $(2)" >&2; exit 1; }; \
	mkdir -p '$(BIN_DIR)/$(1)/windows'; \
	$(2) -std=c11 -O2 -Wall -Wextra -Werror -I"$$target" src/menu.c "$$library" \
		-lole32 -lshell32 -luuid -o "$$out"; \
	echo "OK $(1)/windows"
endef

x86_64/windows:
	$(call windows_target,x86_64,x86_64-w64-mingw32-gcc)

i686/windows:
	$(call windows_target,i686,i686-w64-mingw32-gcc)

test:
	@if [ -n "$(filter wine,$(MAKECMDGOALS))" ]; then \
		$(MAKE) --no-print-directory x86_64/windows; \
		command -v $(WINE) >/dev/null 2>&1 || { echo "kccli: missing Wine runtime: $(WINE)" >&2; exit 1; }; \
		command -v $(WINE_X86_64_CC) >/dev/null 2>&1 || { echo "kccli: missing Windows compiler: $(WINE_X86_64_CC)" >&2; exit 1; }; \
		mkdir -p '$(BUILD_DIR)/test-wine'; \
		cp '$(BIN_DIR)/x86_64/windows/menu.exe' '$(BUILD_DIR)/test-wine/menu.exe'; \
		$(WINE_X86_64_CC) -std=c11 -O2 -Wall -Wextra -Werror \
			-DKC_CLI_PATH=\"menu.exe\" src/test.c \
			-o '$(BUILD_DIR)/test-wine/kccli_test.exe'; \
		cd '$(BUILD_DIR)/test-wine' && \
			EGL_LOG_LEVEL=fatal WINEDEBUG=-all $(WINE) kccli_test.exe; \
	else \
		if [ "$(NATIVE_ARCH)" = "unsupported" ] || [ "$(NATIVE_PLATFORM)" = "unsupported" ]; then \
			echo "kccli: unsupported native target $(HOST_ARCH)/$(HOST_SYSTEM)" >&2; \
			exit 1; \
		fi; \
		$(MAKE) --no-print-directory '$(NATIVE_TARGET)'; \
		mkdir -p '$(BUILD_DIR)/test'; \
		$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
			-DKC_CLI_PATH=\"$(CURDIR)/$(BIN_DIR)/$(NATIVE_TARGET)/menu$(NATIVE_EXE_EXT)\" \
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
	@rm -rf '$(BUILD_DIR)' '$(BIN_DIR)' dist
	@echo "OK clean"

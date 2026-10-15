## Makefile
## Summary: Builds the ngram CLI against precompiled static kclib artifacts.
##
## Author:  KaisarCode
## Website: https://kaisarcode.com
## License: GNU LGPL v3

SHELL := /bin/sh

ifndef KCLIB_DIR
$(error KCLIB_DIR environment variable is required)
endif
NGRAM_PROJECT_DIR := $(KCLIB_DIR)/proj/libngram.c
NGRAM_BIN_DIR := $(NGRAM_PROJECT_DIR)/bin
NGRAM_INCLUDE_DIR := $(NGRAM_PROJECT_DIR)/src
BIN_DIR := bin
BUILD_DIR := .build

ANDROID_HOME ?= $(HOME)/.local/share/android-sdk
NDK_VERSION ?= 27.2.12479018
NDK_DIR := $(ANDROID_HOME)/ndk/$(NDK_VERSION)
NDK_HOST_TAG ?= linux-x86_64
NDK_BIN_DIR := $(NDK_DIR)/toolchains/llvm/prebuilt/$(NDK_HOST_TAG)/bin
ANDROID_API ?= 21
ANDROID_AARCH64_CC := $(NDK_BIN_DIR)/aarch64-linux-android$(ANDROID_API)-clang
ANDROID_ARMV7_CC := $(NDK_BIN_DIR)/armv7a-linux-androideabi$(ANDROID_API)-clang

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
	x86_64/linux x86_64/windows x86_64/macos \
	i686/linux i686/windows \
	aarch64/linux aarch64/android aarch64/macos \
	armv7/linux armv7/android armv7hf/linux \
	riscv64/linux powerpc64le/linux \
	mips/linux mipsel/linux mips64el/linux \
	s390x/linux loongarch64/linux

all: \
	x86_64/linux x86_64/windows x86_64/macos \
	i686/linux i686/windows \
	aarch64/linux aarch64/android aarch64/macos \
	armv7/linux armv7/android armv7hf/linux \
	riscv64/linux powerpc64le/linux \
	mips/linux mipsel/linux mips64el/linux \
	s390x/linux loongarch64/linux

define linux_target
	@target='$(NGRAM_BIN_DIR)/$(1)/linux'; \
	header='$(NGRAM_INCLUDE_DIR)/libngram.h'; \
	library="$$target/libngram.a"; \
	out='$(BIN_DIR)/$(1)/linux/ngram'; \
	[ -f "$$header" ] || { echo "kccli: missing libngram header: $$header" >&2; exit 1; }; \
	[ -f "$$library" ] || { echo "kccli: missing libngram static library: $$library" >&2; exit 1; }; \
	command -v $(2) >/dev/null 2>&1 || { echo "kccli: missing compiler: $(2)" >&2; exit 1; }; \
	mkdir -p '$(BIN_DIR)/$(1)/linux'; \
	$(2) -std=c11 -O2 -Wall -Wextra -Werror -I'$(NGRAM_INCLUDE_DIR)' \
		src/ngram.c "$$library" -o "$$out"; \
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

define android_target
	@target='$(NGRAM_BIN_DIR)/$(1)/android'; \
	header='$(NGRAM_INCLUDE_DIR)/libngram.h'; \
	library="$$target/libngram.a"; \
	out='$(BIN_DIR)/$(1)/android/ngram'; \
	[ -f "$$header" ] || { echo "kccli: missing libngram header: $$header" >&2; exit 1; }; \
	[ -f "$$library" ] || { echo "kccli: missing libngram static library: $$library" >&2; exit 1; }; \
	command -v $(2) >/dev/null 2>&1 || { echo "kccli: missing compiler: $(2)" >&2; exit 1; }; \
	mkdir -p '$(BIN_DIR)/$(1)/android'; \
	$(2) -std=c11 -O2 -Wall -Wextra -Werror -I'$(NGRAM_INCLUDE_DIR)' \
		src/ngram.c "$$library" -o "$$out"; \
	echo "OK $(1)/android"
endef

aarch64/android:
	$(call android_target,aarch64,$(ANDROID_AARCH64_CC))

armv7/android:
	$(call android_target,armv7,$(ANDROID_ARMV7_CC))

define windows_target
	@target='$(NGRAM_BIN_DIR)/$(1)/windows'; \
	header='$(NGRAM_INCLUDE_DIR)/libngram.h'; \
	library="$$target/libngram.a"; \
	out='$(BIN_DIR)/$(1)/windows/ngram.exe'; \
	[ -f "$$header" ] || { echo "kccli: missing libngram header: $$header" >&2; exit 1; }; \
	[ -f "$$library" ] || { echo "kccli: missing libngram static library: $$library" >&2; exit 1; }; \
	command -v $(2) >/dev/null 2>&1 || { echo "kccli: missing compiler: $(2)" >&2; exit 1; }; \
	mkdir -p '$(BIN_DIR)/$(1)/windows'; \
	$(2) -std=c11 -O2 -Wall -Wextra -Werror -I'$(NGRAM_INCLUDE_DIR)' \
		src/ngram.c "$$library" -o "$$out"; \
	echo "OK $(1)/windows"
endef

x86_64/windows:
	$(call windows_target,x86_64,x86_64-w64-mingw32-gcc)

i686/windows:
	$(call windows_target,i686,i686-w64-mingw32-gcc)

define macos_target
	@target='$(NGRAM_BIN_DIR)/$(1)/macos'; \
	header='$(NGRAM_INCLUDE_DIR)/libngram.h'; \
	library="$$target/libngram.a"; \
	out='$(BIN_DIR)/$(1)/macos/ngram'; \
	[ -f "$$header" ] || { echo "kccli: missing libngram header: $$header" >&2; exit 1; }; \
	[ -f "$$library" ] || { echo "kccli: missing libngram static library: $$library" >&2; exit 1; }; \
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
		-mmacosx-version-min="$$min" -I'$(NGRAM_INCLUDE_DIR)' src/ngram.c "$$library" \
		-o "$$out"; \
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
		cp '$(BIN_DIR)/x86_64/windows/ngram.exe' '$(BUILD_DIR)/test-wine/ngram.exe'; \
		$(WINE_X86_64_CC) -std=c11 -O2 -Wall -Wextra -Werror \
			-DKC_CLI_PATH=\"ngram.exe\" src/test.c \
			-o '$(BUILD_DIR)/test-wine/kccli_test.exe'; \
		cd '$(BUILD_DIR)/test-wine' && WINEDEBUG=-all $(WINE) kccli_test.exe; \
	else \
		if [ "$(NATIVE_ARCH)" = "unsupported" ] || [ "$(NATIVE_PLATFORM)" = "unsupported" ]; then \
			echo "kccli: unsupported native target $(HOST_ARCH)/$(HOST_SYSTEM)" >&2; \
			exit 1; \
		fi; \
		$(MAKE) --no-print-directory '$(NATIVE_TARGET)'; \
		mkdir -p '$(BUILD_DIR)/test'; \
		$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
			-DKC_CLI_PATH=\"$(CURDIR)/$(BIN_DIR)/$(NATIVE_TARGET)/ngram$(NATIVE_EXE_EXT)\" \
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

## Makefile
## Summary: Builds the REDP2P WebRTC CLI with external libdatachannel.
##
## Author:  KaisarCode
## Website: https://kaisarcode.com
## License: GNU GPL v3

SHELL := /bin/sh
comma := ,

LIBDATACHANNEL_DIR ?=
XDG_DATA_HOME ?= $(HOME)/.local/share
KC_CROSS_DEPS_DIR ?= $(XDG_DATA_HOME)/kaisarcode/setup/deps
OSXCROSS_ROOT ?= $(XDG_DATA_HOME)/osxcross/target
MACOSX_DEPLOYMENT_TARGET ?= 11.0
DETECTED_MACOSX_SDK := $(shell ls -d "$(OSXCROSS_ROOT)"/SDK/MacOSX*.sdk 2>/dev/null | sort -V | tail -n 1)
MACOSX_SDK ?= $(DETECTED_MACOSX_SDK)
OSXCROSS_X86_64_CC := $(OSXCROSS_ROOT)/bin/o64-clang
OSXCROSS_X86_64_CXX := $(OSXCROSS_ROOT)/bin/o64-clang++
OSXCROSS_AARCH64_CC := $(OSXCROSS_ROOT)/bin/oa64-clang
OSXCROSS_AARCH64_CXX := $(OSXCROSS_ROOT)/bin/oa64-clang++

BIN_DIR := bin
BUILD_DIR := .build
CMAKE ?= cmake
PKG_CONFIG ?= pkg-config
CC ?= cc
CXX ?= c++
WINE ?= wine
WINE_X86_64_CC ?= x86_64-w64-mingw32-gcc
WEBP2P_BUILD_VERSION ?= 0
JOBS ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')

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

.PHONY: all test wine clean rtc \
	x86_64/linux x86_64/windows x86_64/macos \
	i686/linux i686/windows \
	aarch64/linux aarch64/macos \
	armv7/linux armv7hf/linux \
	riscv64/linux powerpc64le/linux \
	mips/linux mipsel/linux mips64el/linux \
	s390x/linux loongarch64/linux

all: \
	x86_64/linux x86_64/windows x86_64/macos \
	i686/linux i686/windows \
	aarch64/linux aarch64/macos \
	armv7/linux armv7hf/linux \
	riscv64/linux powerpc64le/linux \
	mips/linux mipsel/linux mips64el/linux \
	s390x/linux loongarch64/linux

define build_target
	@set -eu; \
	target='$(1)'; \
	system='$(2)'; \
	processor='$(3)'; \
	cc='$(4)'; \
	cxx='$(5)'; \
	exe='$(6)'; \
	platform_libs='$(7)'; \
	rtc_build='$(BUILD_DIR)/libdatachannel/$(1)'; \
	obj_dir='$(BUILD_DIR)/obj/$(1)'; \
	out='$(BIN_DIR)/$(1)/webp2p$(6)'; \
	[ -n "$(LIBDATACHANNEL_DIR)" ] || { echo "webp2p: LIBDATACHANNEL_DIR is required" >&2; exit 1; }; \
	[ -f "$(LIBDATACHANNEL_DIR)/CMakeLists.txt" ] || { echo "webp2p: invalid LIBDATACHANNEL_DIR: $(LIBDATACHANNEL_DIR)" >&2; exit 1; }; \
	command -v "$(CMAKE)" >/dev/null 2>&1 || { echo "webp2p: missing cmake" >&2; exit 1; }; \
	command -v "$$cc" >/dev/null 2>&1 || [ -x "$$cc" ] || { echo "webp2p: missing C compiler: $$cc" >&2; exit 1; }; \
	command -v "$$cxx" >/dev/null 2>&1 || [ -x "$$cxx" ] || { echo "webp2p: missing C++ compiler: $$cxx" >&2; exit 1; }; \
	command -v "$(PKG_CONFIG)" >/dev/null 2>&1 || { echo "webp2p: missing pkg-config" >&2; exit 1; }; \
	deps=''; \
	pc_libdir=''; \
	openssl_args=''; \
	if [ "$$target" = "$(NATIVE_TARGET)" ]; then \
		$(PKG_CONFIG) --exists libcurl openssl || { echo "webp2p: native libcurl/OpenSSL development packages are required" >&2; exit 1; }; \
	else \
		deps='$(KC_CROSS_DEPS_DIR)/$(1)'; \
		[ -f "$$deps/include/curl/curl.h" ] || { echo "webp2p: missing cross libcurl for $$target: $$deps" >&2; exit 1; }; \
		[ -f "$$deps/include/openssl/ssl.h" ] || { echo "webp2p: missing cross OpenSSL for $$target: $$deps" >&2; exit 1; }; \
		pc_libdir="$$deps/lib/pkgconfig:$$deps/lib64/pkgconfig"; \
		PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR="$$pc_libdir" $(PKG_CONFIG) --exists libcurl openssl || { echo "webp2p: invalid cross pkg-config metadata for $$target" >&2; exit 1; }; \
		openssl_args="-DOPENSSL_ROOT_DIR=$$deps -DOPENSSL_USE_STATIC_LIBS=TRUE -DCMAKE_PREFIX_PATH=$$deps"; \
	fi; \
	if [ "$$system" = "Darwin" ]; then \
		[ -d "$(MACOSX_SDK)" ] || { echo "webp2p: missing macOS SDK: $(MACOSX_SDK)" >&2; exit 1; }; \
		export PATH="$(OSXCROSS_ROOT)/bin:$$PATH"; \
		export LD_LIBRARY_PATH="$(OSXCROSS_ROOT)/lib:$${LD_LIBRARY_PATH:-}"; \
	fi; \
	mkdir -p "$$rtc_build" "$$obj_dir" "$(BIN_DIR)/$(1)"; \
	$(CMAKE) \
		-S "$(LIBDATACHANNEL_DIR)" \
		-B "$$rtc_build" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_SYSTEM_NAME="$$system" \
		-DCMAKE_SYSTEM_PROCESSOR="$$processor" \
		-DCMAKE_C_COMPILER="$$cc" \
		-DCMAKE_CXX_COMPILER="$$cxx" \
		$(8) \
		$$openssl_args \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_SHARED_DEPS_LIBS=OFF \
		-DNO_MEDIA=ON \
		-DNO_WEBSOCKET=ON \
		-DNO_EXAMPLES=ON \
		-DNO_TESTS=ON \
		-DUSE_GNUTLS=OFF \
		-DUSE_MBEDTLS=OFF \
		-DUSE_NICE=OFF \
		-DWARNINGS_AS_ERRORS=OFF; \
	$(CMAKE) --build "$$rtc_build" --target datachannel-static -j "$(JOBS)"; \
	rtc_inc='$(LIBDATACHANNEL_DIR)/include'; \
	datachannel=$$(find "$$rtc_build" -type f -name 'libdatachannel-static.a' -print -quit); \
	juice=$$(find "$$rtc_build" -type f \( -name 'libjuice-static.a' -o -name 'libjuice.a' \) -print -quit); \
	usrsctp=$$(find "$$rtc_build" -type f -name 'libusrsctp.a' -print -quit); \
	[ -f "$$rtc_inc/rtc/rtc.h" ] || { echo "webp2p: missing rtc/rtc.h" >&2; exit 1; }; \
	[ -n "$$datachannel" ] || { echo "webp2p: libdatachannel-static.a not produced for $$target" >&2; exit 1; }; \
	[ -n "$$juice" ] || { echo "webp2p: libjuice static archive not produced for $$target" >&2; exit 1; }; \
	[ -n "$$usrsctp" ] || { echo "webp2p: libusrsctp.a not produced for $$target" >&2; exit 1; }; \
	if [ -n "$$pc_libdir" ]; then \
		pc_cflags=$$(PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR="$$pc_libdir" $(PKG_CONFIG) --cflags libcurl openssl); \
		pc_libs=$$(PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR="$$pc_libdir" $(PKG_CONFIG) --static --libs libcurl openssl); \
	else \
		pc_cflags=$$($(PKG_CONFIG) --cflags libcurl openssl); \
		pc_libs=$$($(PKG_CONFIG) --static --libs libcurl openssl); \
	fi; \
	cflags="$$pc_cflags -I$$rtc_inc -Isrc -DRTC_STATIC -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections $(9)"; \
	"$$cc" $$cflags '-DWEBP2P_BUILD_VERSION="$(WEBP2P_BUILD_VERSION)"' -c src/webp2p.c -o "$$obj_dir/webp2p.o"; \
	"$$cc" $$cflags '-DWEBP2P_BUILD_VERSION="$(WEBP2P_BUILD_VERSION)"' -c src/protocol.c -o "$$obj_dir/protocol.o"; \
	"$$cc" $$cflags '-DWEBP2P_BUILD_VERSION="$(WEBP2P_BUILD_VERSION)"' -c src/rtc.c -o "$$obj_dir/rtc.o"; \
	"$$cxx" "$$obj_dir/webp2p.o" "$$obj_dir/protocol.o" "$$obj_dir/rtc.o" \
		"$$datachannel" "$$juice" "$$usrsctp" $$pc_libs $$platform_libs \
		$(10) -o "$$out"; \
	echo "OK $$target"
endef

## Linux

x86_64/linux:
	$(call build_target,x86_64/linux,Linux,x86_64,gcc,g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

i686/linux:
	$(call build_target,i686/linux,Linux,i686,i686-linux-gnu-gcc,i686-linux-gnu-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

aarch64/linux:
	$(call build_target,aarch64/linux,Linux,aarch64,aarch64-linux-gnu-gcc,aarch64-linux-gnu-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

armv7/linux:
	$(call build_target,armv7/linux,Linux,armv7,arm-linux-gnueabi-gcc,arm-linux-gnueabi-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

armv7hf/linux:
	$(call build_target,armv7hf/linux,Linux,armv7,arm-linux-gnueabihf-gcc,arm-linux-gnueabihf-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

riscv64/linux:
	$(call build_target,riscv64/linux,Linux,riscv64,riscv64-linux-gnu-gcc,riscv64-linux-gnu-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

powerpc64le/linux:
	$(call build_target,powerpc64le/linux,Linux,ppc64le,powerpc64le-linux-gnu-gcc,powerpc64le-linux-gnu-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

mips/linux:
	$(call build_target,mips/linux,Linux,mips,mips-linux-gnu-gcc,mips-linux-gnu-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

mipsel/linux:
	$(call build_target,mipsel/linux,Linux,mipsel,mipsel-linux-gnu-gcc,mipsel-linux-gnu-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

mips64el/linux:
	$(call build_target,mips64el/linux,Linux,mips64el,mips64el-linux-gnuabi64-gcc,mips64el-linux-gnuabi64-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

s390x/linux:
	$(call build_target,s390x/linux,Linux,s390x,s390x-linux-gnu-gcc,s390x-linux-gnu-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

loongarch64/linux:
	$(call build_target,loongarch64/linux,Linux,loongarch64,loongarch64-linux-gnu-gcc,loongarch64-linux-gnu-g++,,-pthread -ldl -lm,,-D_GNU_SOURCE,-Wl$(comma)--gc-sections)

## Windows

x86_64/windows:
	$(call build_target,x86_64/windows,Windows,x86_64,x86_64-w64-mingw32-gcc,x86_64-w64-mingw32-g++,.exe,-lwinpthread -lws2_32 -liphlpapi -lbcrypt -lcrypt32,,-D_WIN32_WINNT=0x0601,-Wl$(comma)--gc-sections)

i686/windows:
	$(call build_target,i686/windows,Windows,i686,i686-w64-mingw32-gcc,i686-w64-mingw32-g++,.exe,-lwinpthread -lws2_32 -liphlpapi -lbcrypt -lcrypt32,,-D_WIN32_WINNT=0x0601,-Wl$(comma)--gc-sections)

## macOS

x86_64/macos:
	$(call build_target,x86_64/macos,Darwin,x86_64,$(OSXCROSS_X86_64_CC),$(OSXCROSS_X86_64_CXX),,-pthread -lm,-DCMAKE_OSX_SYSROOT="$(MACOSX_SDK)" -DCMAKE_OSX_DEPLOYMENT_TARGET="$(MACOSX_DEPLOYMENT_TARGET)",,-Wl$(comma)-dead_strip)

aarch64/macos:
	$(call build_target,aarch64/macos,Darwin,arm64,$(OSXCROSS_AARCH64_CC),$(OSXCROSS_AARCH64_CXX),,-pthread -lm,-DCMAKE_OSX_SYSROOT="$(MACOSX_SDK)" -DCMAKE_OSX_DEPLOYMENT_TARGET="$(MACOSX_DEPLOYMENT_TARGET)",,-Wl$(comma)-dead_strip)

rtc:
	@$(MAKE) --no-print-directory '$(NATIVE_TARGET)'

test:
	@if [ -n "$(filter wine,$(MAKECMDGOALS))" ]; then \
		$(MAKE) --no-print-directory x86_64/windows || exit 1; \
		command -v $(WINE) >/dev/null 2>&1 || { echo "webp2p: missing Wine runtime: $(WINE)" >&2; exit 1; }; \
		command -v $(WINE_X86_64_CC) >/dev/null 2>&1 || { echo "webp2p: missing Windows compiler: $(WINE_X86_64_CC)" >&2; exit 1; }; \
		mkdir -p '$(BUILD_DIR)/test-wine'; \
		cp '$(BIN_DIR)/x86_64/windows/webp2p.exe' '$(BUILD_DIR)/test-wine/webp2p.exe'; \
		$(WINE_X86_64_CC) -std=c11 -O2 -Wall -Wextra -Werror \
			-DKC_CLI_PATH=\"webp2p.exe\" src/test.c \
			-o '$(BUILD_DIR)/test-wine/webp2p_test.exe'; \
		cd '$(BUILD_DIR)/test-wine' && \
			EGL_LOG_LEVEL=fatal WINEDEBUG=-all $(WINE) webp2p_test.exe; \
	else \
		if [ "$(NATIVE_ARCH)" = "unsupported" ] || [ "$(NATIVE_PLATFORM)" = "unsupported" ]; then \
			echo "webp2p: unsupported native target $(HOST_ARCH)/$(HOST_SYSTEM)" >&2; \
			exit 1; \
		fi; \
		$(MAKE) --no-print-directory '$(NATIVE_TARGET)' || exit 1; \
		mkdir -p '$(BUILD_DIR)/test'; \
		$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
			-DKC_CLI_PATH=\"$(CURDIR)/$(BIN_DIR)/$(NATIVE_TARGET)/webp2p$(NATIVE_EXE_EXT)\" \
			src/test.c -o '$(BUILD_DIR)/test/webp2p_test'; \
		'$(BUILD_DIR)/test/webp2p_test'; \
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

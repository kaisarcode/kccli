## Makefile
## Summary: Builds the native REDP2P WebRTC CLI with external libdatachannel.
##
## Author:  KaisarCode
## Website: https://kaisarcode.com
## License: GNU GPL v3

SHELL := /bin/sh

LIBDATACHANNEL_DIR ?=
BIN_DIR := bin
BUILD_DIR := .build
RTC_BUILD_DIR := $(BUILD_DIR)/libdatachannel
OBJ_DIR := $(BUILD_DIR)/obj
CC ?= cc
CXX ?= c++
CMAKE ?= cmake
PKG_CONFIG ?= pkg-config
WEBP2P_BUILD_VERSION ?= 0
JOBS ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')

HOST_ARCH := $(shell uname -m)
HOST_SYSTEM := $(shell uname -s)
NATIVE_ARCH := $(HOST_ARCH)

ifneq ($(filter amd64,$(NATIVE_ARCH)),)
NATIVE_ARCH := x86_64
endif
ifneq ($(filter arm64,$(NATIVE_ARCH)),)
NATIVE_ARCH := aarch64
endif

NATIVE_TARGET := $(NATIVE_ARCH)/linux
OUT := $(BIN_DIR)/$(NATIVE_TARGET)/webp2p

.DEFAULT_GOAL := $(NATIVE_TARGET)

.PHONY: all test clean $(NATIVE_TARGET) rtc check

all: $(NATIVE_TARGET)

check:
	@[ "$(HOST_SYSTEM)" = "Linux" ] || { \
		echo "webp2p: current build supports Linux desktop only" >&2; exit 1; }
	@[ -n "$(LIBDATACHANNEL_DIR)" ] || { \
		echo "webp2p: LIBDATACHANNEL_DIR is required" >&2; exit 1; }
	@[ -f "$(LIBDATACHANNEL_DIR)/CMakeLists.txt" ] || { \
		echo "webp2p: invalid LIBDATACHANNEL_DIR: $(LIBDATACHANNEL_DIR)" >&2; exit 1; }
	@command -v $(CMAKE) >/dev/null 2>&1 || { \
		echo "webp2p: missing cmake" >&2; exit 1; }
	@command -v $(CC) >/dev/null 2>&1 || { \
		echo "webp2p: missing C compiler: $(CC)" >&2; exit 1; }
	@command -v $(CXX) >/dev/null 2>&1 || { \
		echo "webp2p: missing C++ compiler: $(CXX)" >&2; exit 1; }
	@command -v $(PKG_CONFIG) >/dev/null 2>&1 || { \
		echo "webp2p: missing pkg-config" >&2; exit 1; }
	@$(PKG_CONFIG) --exists libcurl openssl || { \
		echo "webp2p: libcurl and OpenSSL development packages are required" >&2; exit 1; }

rtc: check
	@mkdir -p '$(RTC_BUILD_DIR)'
	@$(CMAKE) \
		-S '$(LIBDATACHANNEL_DIR)' \
		-B '$(RTC_BUILD_DIR)' \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER='$(CC)' \
		-DCMAKE_CXX_COMPILER='$(CXX)' \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_SHARED_DEPS_LIBS=OFF \
		-DNO_MEDIA=ON \
		-DNO_WEBSOCKET=ON \
		-DNO_EXAMPLES=ON \
		-DNO_TESTS=ON \
		-DUSE_GNUTLS=OFF \
		-DUSE_MBEDTLS=OFF \
		-DUSE_NICE=OFF
	@$(CMAKE) --build '$(RTC_BUILD_DIR)' --target datachannel-static -j '$(JOBS)'

$(NATIVE_TARGET): rtc
	@set -eu; \
	rtc_inc='$(LIBDATACHANNEL_DIR)/include'; \
	datachannel=$$(find '$(RTC_BUILD_DIR)' -type f -name 'libdatachannel-static.a' -print -quit); \
	juice=$$(find '$(RTC_BUILD_DIR)' -type f \( -name 'libjuice-static.a' -o -name 'libjuice.a' \) -print -quit); \
	usrsctp=$$(find '$(RTC_BUILD_DIR)' -type f -name 'libusrsctp.a' -print -quit); \
	[ -f "$$rtc_inc/rtc/rtc.h" ] || { echo "webp2p: missing rtc/rtc.h" >&2; exit 1; }; \
	[ -n "$$datachannel" ] || { echo "webp2p: libdatachannel-static.a not produced" >&2; exit 1; }; \
	[ -n "$$juice" ] || { echo "webp2p: libjuice static archive not produced" >&2; exit 1; }; \
	[ -n "$$usrsctp" ] || { echo "webp2p: libusrsctp.a not produced" >&2; exit 1; }; \
	mkdir -p '$(OBJ_DIR)' '$(BIN_DIR)/$(NATIVE_TARGET)'; \
	cflags="$$($(PKG_CONFIG) --cflags libcurl openssl) -I$$rtc_inc -Isrc -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -DWEBP2P_BUILD_VERSION=\\\"$(WEBP2P_BUILD_VERSION)\\\""; \
	$(CC) $$cflags -c src/webp2p.c -o '$(OBJ_DIR)/webp2p.o'; \
	$(CC) $$cflags -c src/protocol.c -o '$(OBJ_DIR)/protocol.o'; \
	$(CC) $$cflags -c src/rtc.c -o '$(OBJ_DIR)/rtc.o'; \
	libs="$$($(PKG_CONFIG) --libs libcurl openssl) -lpthread -ldl -lm"; \
	$(CXX) '$(OBJ_DIR)/webp2p.o' '$(OBJ_DIR)/protocol.o' '$(OBJ_DIR)/rtc.o' \
		"$$datachannel" "$$juice" "$$usrsctp" $$libs \
		-Wl,--gc-sections -o '$(OUT)'; \
	echo "OK $(NATIVE_TARGET)"

test: $(NATIVE_TARGET)
	@mkdir -p '$(BUILD_DIR)/test'
	@$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-DKC_CLI_PATH=\"$(CURDIR)/$(OUT)\" \
		src/test.c -o '$(BUILD_DIR)/test/webp2p_test'
	@'$(BUILD_DIR)/test/webp2p_test'

clean:
	@rm -rf '$(BUILD_DIR)' '$(BIN_DIR)'
	@echo "OK clean"

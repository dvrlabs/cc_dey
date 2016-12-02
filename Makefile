# ***************************************************************************
# Copyright (c) 2017 Digi International Inc.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#
# Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
#
# ***************************************************************************
# Use GNU C Compiler
CC ?= gcc

# Generated Library name.
NAME := cloud
# Generated Library version.
MAJOR := 1
MINOR := 0
REVISION := 0
VERSION := $(MAJOR).$(MINOR).$(REVISION)

# Location of Source Code.
SRC = src

# Location of CC API dir.
CCAPI_DIR = $(SRC)/cc_api

# Location of Connector dir.
CCFSM_DIR = $(CCAPI_DIR)/source/cc_ansic

# Location of Private Connector Source Code.
CCAPI_PRIVATE_DIR = $(CCAPI_DIR)/source
CCFSM_PRIVATE_DIR = $(CCFSM_DIR)/private

# Location of Public Include Header Files.
CCFSM_PUBLIC_HEADER_DIR = $(CCFSM_DIR)/public/include
CCAPI_PUBLIC_HEADER_DIR = $(CCAPI_DIR)/include
CUSTOM_PUBLIC_HEADER_DIR = $(SRC)/custom
CUSTOM_CCFSM_PUBLIC_HEADER_DIR = $(CCAPI_DIR)/source/cc_ansic_custom_include

# Location of Platform Source Code.
PLATFORM_DIR = $(SRC)/ccimp

# Resolves where to find Source files.
vpath $(CCFSM_PRIVATE_DIR)/%.c
vpath $(PLATFORM_DIR)/%.c

# CFLAG Definition
CFLAGS += -I $(SRC) -I $(CUSTOM_CCFSM_PUBLIC_HEADER_DIR) -I $(CCFSM_PUBLIC_HEADER_DIR)
CFLAGS += -I $(CCAPI_PUBLIC_HEADER_DIR) -I $(CUSTOM_PUBLIC_HEADER_DIR) -I $(PLATFORM_DIR)
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200112L -D_GNU_SOURCE
CFLAGS += -Wall -Wextra -fPIC -g -O0

# Target output to generate.
CC_PRIVATE_SRCS := $(CCFSM_PRIVATE_DIR)/connector_api.c
CCAPI_PRIVATE_SRCS := $(wildcard $(CCAPI_PRIVATE_DIR)/*.c)
PLATFORM_SRCS := $(wildcard $(PLATFORM_DIR)/*.c)
CC_DEY_SRCS := $(wildcard $(SRC)/*.c)

SRCS = $(CC_DEY_SRCS) $(PLATFORM_SRCS) $(CC_PRIVATE_SRCS) $(CCAPI_PRIVATE_SRCS)

# Libraries to Link
LDLIBS += -lconfuse -lpthread -lcrypto -lz

# Linking Flags.
LDFLAGS += $(DFLAGS) -shared -Wl,-soname,lib$(NAME).so.$(MAJOR),--sort-common

OBJS = $(SRCS:.c=.o)

.PHONY: all
all:  lib$(NAME).so

lib$(NAME).so: lib$(NAME).so.$(VERSION)
	ln -sf lib$(NAME).so.$(VERSION) lib$(NAME).so.$(MAJOR)
	ln -sf lib$(NAME).so.$(VERSION) lib$(NAME).so

lib$(NAME).so.$(VERSION): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: clean
clean:
	-rm -f *.so* $(OBJS)

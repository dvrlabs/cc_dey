# ***************************************************************************
# Copyright (c) 2017 Digi International Inc.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
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

CPP = g++
CC = gcc

# Target Platform
CCAPI_SOURCE_DIR = source
CCAPI_INCLUDE = include
CONNECTOR_DIR = $(CCAPI_SOURCE_DIR)/cc_ansic
CUSTOM_CONNECTOR_INCLUDE = $(CCAPI_SOURCE_DIR)/cc_ansic_custom_include
CONNECTOR_INCLUDE = $(CONNECTOR_DIR)/public/include
UNIT_TEST_INCLUDE = tests/unit_tests

CONFIG_GENERATOR_BUILD = $(CONNECTOR_DIR)/tools/config/build.xml
CONFIG_GENERATOR_BIN = $(CONNECTOR_DIR)/tools/config/dist/ConfigGenerator.jar

TEST_DIR = tests/unit_tests
MOCKS_DIR = tests/mocks
CCIMP_SOURCE_DIR = tests/ccimp

# CFLAG Definition
CFLAGS += $(DFLAGS)
# Enable Compiler Warnings
CFLAGS += -Winit-self -Wpointer-arith
CFLAGS += -Wformat-security
CFLAGS += -Wformat-y2k -Wcast-align -Wformat-nonliteral
CFLAGS += -Wredundant-decls -Wvariadic-macros
CFLAGS += -Wall -Werror -Wextra -pedantic
CFLAGS += -Wno-error=format-nonliteral -Wno-unused-function -Wno-missing-field-initializers
CFLAGS += -Wno-error=deprecated-declarations -Wno-error=incompatible-pointer-types -Wno-error=int-conversion -Wno-error=discarded-qualifiers
CFLAGS += --coverage

# Include POSIX and GNU features.
CFLAGS += -D_POSIX_C_SOURCE=200112L -D_GNU_SOURCE
# Include Public Header Files.
CFLAGS += -iquote$(UNIT_TEST_INCLUDE) -iquote$(MOCKS_DIR) -iquote$(CCAPI_INCLUDE) -iquote. -iquote$(CUSTOM_CONNECTOR_INCLUDE) -iquote$(CONNECTOR_INCLUDE) -iquote$(CCAPI_SOURCE_DIR)
CFLAGS += -g -O0

CCAPI_SOURCES = $(wildcard $(CCAPI_SOURCE_DIR)/*.c)
CCIMP_SOURCES = $(wildcard $(CCIMP_SOURCE_DIR)/*.c) 
TESTS_SOURCES := $(shell find $(TEST_DIR) -name '*.cpp')
MOCKS_SOURCES = $(wildcard $(MOCKS_DIR)/*.cpp)

RCI_TESTS_SOURCES = $(wildcard $(TEST_DIR)/ccapi_rci/*.c)

CSRCS = $(CCAPI_SOURCES) $(CCIMP_SOURCES) $(RCI_TESTS_SOURCES)

CPPSRCS = $(wildcard ./*.cpp) $(TESTS_SOURCES) $(MOCKS_SOURCES)

# Libraries to Link
LIBS = -lc -lCppUTest -lCppUTestExt -lpthread -lrt -lcrypto

CCFLAGS := $(CFLAGS) -std=c99

CFLAGS += -std=c++0x

# Generated Sample Executable Name.
EXEC_NAME = test

# since each of the samples shares private and platform files, do a clean each time we make
.PHONY:all
all: test

# Linking Flags.
LDFLAGS += $(DFLAGS) -Wl,-Map,$(EXEC_NAME).map,--sort-common --coverage

COBJS = $(CSRCS:.c=.o)
CPPOBJS = $(CPPSRCS:.cpp=.o)
GCOVOBJS = $(CSRCS:.c=.gcda) $(CSRCS:.c=.gcno) $(CPPSRCS:.cpp=.gcda) $(CPPSRCS:.cpp=.gcno)

ConfigGenerator:
	ant -f $(CONFIG_GENERATOR_BUILD)

auto_generated_files: ConfigGenerator
	# java -jar $(CONFIG_GENERATOR_BIN) -path=$(CUSTOM_CONNECTOR_INCLUDE) -noBackup -ccapi -rci_legacy_commands
	java -jar $(CONFIG_GENERATOR_BIN) username:password "Device type" 1.0.0.0 -vendor=0x12345678 -path=$(TEST_DIR)/ccapi_rci -noUpload -noBackup -ccapi -c99 -usenames=all -rci_legacy_commands $(TEST_DIR)/ccapi_rci/config.rci
	{ \
		set -e; \
		for file in connector_api_remote.h; do \
			cp $(TEST_DIR)/ccapi_rci/$$file $(CUSTOM_CONNECTOR_INCLUDE); \
		done; \
	}
	
test_binary: $(COBJS) $(CPPOBJS)
	$(CPP) -DUNIT_TEST $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $(EXEC_NAME)
	
test: auto_generated_files test_binary


run_test: test
	@-./$(EXEC_NAME)

coverage: run_test
	@-gcovr -r . --gcov-filter="$(CCAPI_SOURCE_DIR)/*"
		
.cpp.o:
	$(CPP) -DUNIT_TEST $(CFLAGS) -c $< -o $@

.c.o:
	$(CC) -DUNIT_TEST $(CCFLAGS) -c $< -o $@

.PHONY: clean
clean:
	-$(RM) -f $(COBJS) $(CPPOBJS) $(GCOVOBJS) $(EXEC_NAME)

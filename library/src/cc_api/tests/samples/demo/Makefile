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
CC = gcc

# Location of Private Connector Code.
CCFSM_PRIVATE_DIR=../../../source/cc_ansic/private
CCAPI_PRIVATE_DIR=../../../source
# Locate of Public Include Header Files.
CCFSM_PUBLIC_HEADER_DIR=../../../source/cc_ansic/public/include
CCAPI_PUBLIC_HEADER_DIR=../../../include
CUSTOM_CCFSM_PUBLIC_HEADER_DIR=../../../source/cc_ansic_custom_include

# Location of Platform Src Code.
PLATFORM_DIR=../../ccimp

# Resolves where to find Source files.
vpath $(CCFSM_PRIVATE_DIR)/%.c
vpath $(PLATFORM_DIR)/%.c

# CFLAG Definition
CFLAGS += $(DFLAGS)
# Enable Compiler Warnings
CFLAGS += -Winit-self -Wbad-function-cast -Wpointer-arith
CFLAGS += -Wmissing-parameter-type -Wstrict-prototypes -Wformat-security 
CFLAGS += -Wformat-y2k -Wold-style-definition -Wcast-align -Wformat-nonliteral 
CFLAGS += -Wpadded -Wredundant-decls -Wvariadic-macros
CFLAGS += -Wall -Werror -Wextra -pedantic
CFLAGS += -Wno-error=padded -Wno-error=format-nonliteral -Wno-unused-function -Wno-missing-field-initializers 
# Use ANSIC 99
CFLAGS +=-std=c99 
# Include POSIX and GNU features.
CFLAGS += -D_POSIX_C_SOURCE=200112L -D_GNU_SOURCE 
# Include Public Header Files.
CFLAGS += -iquote. -iquote$(CUSTOM_CCFSM_PUBLIC_HEADER_DIR) -iquote$(CCFSM_PUBLIC_HEADER_DIR) -iquote$(CCAPI_PUBLIC_HEADER_DIR)
# Include Platform Header Files.
CFLAGS += -iquote$(PLATFORM_DIR)


CFLAGS += -g -O0 -DDEBUG

# Target output to generate.
APP_SRCS = main.c simulated_tank.c device_id_utils.c
CC_PRIVATE_SRCS = $(CCFSM_PRIVATE_DIR)/connector_api.c

CCAPI_PRIVATE_SRCS := $(CCAPI_PRIVATE_DIR)/ccapi.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_init.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_logging.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_transport_tcp.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_transport_udp.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_transport_sms.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_datapoints.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_send.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_receive.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_data_handler.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_datapoints_binary.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_filesystem.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_filesystem_handler.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_firmware_update_handler.c
CCAPI_PRIVATE_SRCS += $(CCAPI_PRIVATE_DIR)/ccapi_sm_handler.c

PLATFORM_SRCS := $(PLATFORM_DIR)/ccimp_filesystem.c
PLATFORM_SRCS += $(PLATFORM_DIR)/crc_32.c
PLATFORM_SRCS += $(PLATFORM_DIR)/ccimp_hal.c  
PLATFORM_SRCS += $(PLATFORM_DIR)/ccimp_logging.c  
PLATFORM_SRCS += $(PLATFORM_DIR)/ccimp_network_sms.c  
PLATFORM_SRCS += $(PLATFORM_DIR)/ccimp_network_tcp.c  
PLATFORM_SRCS += $(PLATFORM_DIR)/ccimp_network_udp.c  
PLATFORM_SRCS += $(PLATFORM_DIR)/ccimp_os.c  
PLATFORM_SRCS += $(PLATFORM_DIR)/dns_helper.c 

SRCS = $(APP_SRCS) $(PLATFORM_SRCS) $(CC_PRIVATE_SRCS) $(CCAPI_PRIVATE_SRCS)

# Libraries to Link
LIBS = -lc -lz -lpthread -lrt -lcrypto

# Generated Sample Executable Name.
SAMPLE = ccapi_app

# since each of the samples shares private and platform files, do a clean each time we make
.PHONY:all
all: clean $(SAMPLE)

# Linking Flags.
LDFLAGS += $(DFLAGS) -Wl,-Map,$(SAMPLE).map,--sort-common

OBJS = $(SRCS:.c=.o)

$(SAMPLE): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@
	


.PHONY: clean
clean:
	-rm -f $(SAMPLE) $(OBJS) $(SAMPLE).map

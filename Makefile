# ***************************************************************************
# Copyright (C) 2016 Digi International Inc., All Rights Reserved
#
# This software contains proprietary and confidential information of Digi.
# International Inc. By accepting transfer of this copy, Recipient agrees to
# retain this software in confidence, to prevent disclosure to others, and to
# make no use of this software other than that for which it was delivered.
# This is an unpublished copyrighted work of Digi International Inc. Except as
# permitted by federal law, 17 USC 117, copying is strictly prohibited.
#
# Restricted Rights Legend
#
# Use, duplication, or disclosure by the Government is subject to restrictions
# set forth in sub-paragraph (c)(1)(ii) of The Rights in Technical Data and
# Computer Software clause at DFARS 252.227-7031 or subparagraphs (c)(1) and (2)
# of the Commercial Computer Software - Restricted Rights at 48 CFR 52.227-19,
# as applicable.
#
# Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
#
# ***************************************************************************

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
CFLAGS += -Wall -Wextra -O2

# Target output to generate.
APP_SRCS = $(SRC)/main.c

CC_PRIVATE_SRCS = $(CCFSM_PRIVATE_DIR)/connector_api.c

CCAPI_PRIVATE_SRCS = 	$(CCAPI_PRIVATE_DIR)/ccapi.c \
						$(CCAPI_PRIVATE_DIR)/ccapi_data_handler.c \
						$(CCAPI_PRIVATE_DIR)/ccapi_datapoints.c \
						$(CCAPI_PRIVATE_DIR)/ccapi_filesystem.c \
						$(CCAPI_PRIVATE_DIR)/ccapi_filesystem_handler.c \
						$(CCAPI_PRIVATE_DIR)/ccapi_firmware_update_handler.c \
						$(CCAPI_PRIVATE_DIR)/ccapi_init.c \
						$(CCAPI_PRIVATE_DIR)/ccapi_logging.c \
						$(CCAPI_PRIVATE_DIR)/ccapi_receive.c \
						$(CCAPI_PRIVATE_DIR)/ccapi_transport_tcp.c

PLATFORM_SRCS = $(PLATFORM_DIR)/ccimp_datapoints.c \
				$(PLATFORM_DIR)/ccimp_filesystem.c \
				$(PLATFORM_DIR)/ccimp_hal.c \
				$(PLATFORM_DIR)/ccimp_logging.c \
				$(PLATFORM_DIR)/ccimp_network_tcp.c \
				$(PLATFORM_DIR)/ccimp_os.c \
				$(PLATFORM_DIR)/crc_32.c \
				$(PLATFORM_DIR)/dns_helper.c

SRCS = $(APP_SRCS) $(PLATFORM_SRCS) $(CC_PRIVATE_SRCS) $(CCAPI_PRIVATE_SRCS)

# Libraries to Link
LDLIBS += -lpthread -lcrypto -lz

# Linking Flags.
LDFLAGS += $(DFLAGS) -Wl,-Map,$(EXECUTABLE).map,--sort-common

# Generated Executable Name.
EXECUTABLE = cc_dey

OBJS = $(SRCS:.c=.o)

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: clean
clean:
	-rm -f $(EXECUTABLE) $(OBJS) $(EXECUTABLE).map


/*
 * Copyright (c) 2017 Digi International Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

#ifndef CONNECTOR_API_REMOTE_H
#define CONNECTOR_API_REMOTE_H
#define RCI_PARSER_USES_ERROR_DESCRIPTIONS
#define RCI_PARSER_USES_STRING
#define RCI_PARSER_USES_MULTILINE_STRING
#define RCI_PARSER_USES_PASSWORD
#define RCI_PARSER_USES_INT32
#define RCI_PARSER_USES_UINT32
#define RCI_PARSER_USES_HEX32
#define RCI_PARSER_USES_0X_HEX32
#define RCI_PARSER_USES_FLOAT
#define RCI_PARSER_USES_ENUM
#define RCI_PARSER_USES_ON_OFF
#define RCI_PARSER_USES_BOOLEAN
#define RCI_PARSER_USES_IPV4
#define RCI_PARSER_USES_FQDNV4
#define RCI_PARSER_USES_FQDNV6
#define RCI_PARSER_USES_MAC_ADDR
#define RCI_PARSER_USES_DATETIME
#define RCI_PARSER_USES_UNSIGNED_INTEGER
#define RCI_PARSER_USES_STRINGS

#include "float.h"

#define RCI_LEGACY_COMMANDS

#define RCI_ENUMS_AS_STRINGS
#define RCI_COMMANDS_ATTRIBUTE_MAX_LEN 20

typedef enum {
	connector_off, connector_on
} connector_on_off_t;

typedef enum {
	connector_element_type_string = 1,
	connector_element_type_multiline_string,
	connector_element_type_password,
	connector_element_type_int32,
	connector_element_type_uint32,
	connector_element_type_hex32,
	connector_element_type_0x_hex32,
	connector_element_type_float,
	connector_element_type_enum,
	connector_element_type_on_off = 11,
	connector_element_type_boolean,
	connector_element_type_ipv4,
	connector_element_type_fqdnv4,
	connector_element_type_fqdnv6,
	connector_element_type_mac_addr = 21,
	connector_element_type_datetime
} connector_element_value_type_t;

typedef struct {
	size_t min_length_in_bytes;
	size_t max_length_in_bytes;
} connector_element_value_string_t;

typedef struct {
	int32_t min_value;
	int32_t max_value;
} connector_element_value_signed_integer_t;

typedef struct {
	uint32_t min_value;
	uint32_t max_value;
} connector_element_value_unsigned_integer_t;

typedef struct {
	float min_value;
	float max_value;
} connector_element_value_float_t;

typedef struct {
	size_t count;
} connector_element_value_enum_t;

typedef union {
	char const * string_value;
	int32_t signed_integer_value;
	uint32_t unsigned_integer_value;
	float float_value;
	unsigned int enum_value;
	connector_on_off_t on_off_value;
	connector_bool_t boolean_value;
} connector_element_value_t;

#define STRING_VALUE
#define SIGNED_INTEGER_VALUE
#define UNSIGNED_INTEGER_VALUE
#define FLOAT_VALUE
#define ENUM_VALUE
#define ON_OFF_VALUE
#define BOOLEAN_VALUE

typedef enum {
	connector_request_id_remote_config_session_start,
	connector_request_id_remote_config_action_start,
	connector_request_id_remote_config_group_start,
	connector_request_id_remote_config_group_process,
	connector_request_id_remote_config_group_end,
	connector_request_id_remote_config_action_end,
	connector_request_id_remote_config_session_end,
	connector_request_id_remote_config_session_cancel,
	connector_request_id_remote_config_do_command,
	connector_request_id_remote_config_reboot,
	connector_request_id_remote_config_set_factory_def
} connector_request_id_remote_config_t;

typedef enum {
	connector_remote_action_set,
	connector_remote_action_query,
	connector_remote_action_do_command,
	connector_remote_action_reboot,
	connector_remote_action_set_factory_def
} connector_remote_action_t;

typedef enum {
	connector_remote_group_setting, connector_remote_group_state
} connector_remote_group_type_t;

typedef enum {
	connector_element_access_read_only,
	connector_element_access_write_only,
	connector_element_access_read_write
} connector_element_access_t;

typedef struct {
	char const * const name;
} connector_element_enum_t;

typedef struct {
	connector_element_access_t access;
	connector_element_value_type_t type;
	struct {
		size_t count;connector_element_enum_t CONST * CONST data;
	} enums;
} connector_group_element_t;

typedef struct {
	size_t instances;
	struct {
		size_t count;connector_group_element_t CONST * CONST data;
	} elements;

	struct {
		size_t count;char CONST * CONST * description;
	} errors;

} connector_group_t;

typedef struct {
	connector_remote_group_type_t type;
	unsigned int id;
	unsigned int index;
} connector_remote_group_t;

typedef struct {
	unsigned int id;
	connector_element_value_type_t type;
	connector_element_value_t * value;
} connector_remote_element_t;

typedef enum {
	rci_query_setting_attribute_source_current,
	rci_query_setting_attribute_source_stored,
	rci_query_setting_attribute_source_defaults
} rci_query_setting_attribute_source_t;

typedef enum {
	rci_query_setting_attribute_compare_to_none,
	rci_query_setting_attribute_compare_to_current,
	rci_query_setting_attribute_compare_to_stored,
	rci_query_setting_attribute_compare_to_defaults
} rci_query_setting_attribute_compare_to_t;

typedef struct {
	rci_query_setting_attribute_source_t source;
	rci_query_setting_attribute_compare_to_t compare_to;
	char const * target;
} connector_remote_attribute_t;

typedef enum {
	rci_query_setting_attribute_id_source,
	rci_query_setting_attribute_id_compare_to,
	rci_query_setting_attribute_id_count
} rci_query_setting_attribute_id_t;

typedef struct {
	void * user_context;connector_remote_action_t
	CONST action;connector_remote_attribute_t
	CONST attribute;connector_remote_group_t
	CONST group;connector_remote_element_t
	CONST element;
	unsigned int error_id;

	struct {
		connector_bool_t compare_matches;
		char const * error_hint;
		connector_element_value_t * element_value;
	} response;
} connector_remote_config_t;

typedef struct {
	void * user_context;
} connector_remote_config_cancel_t;

typedef struct connector_remote_group_table {
	connector_group_t
	CONST * groups;
	size_t count;
} connector_remote_group_table_t;

typedef enum {
	connector_rci_error_OFFSET = 1,
	connector_rci_error_bad_command = connector_rci_error_OFFSET,
	connector_rci_error_bad_descriptor,
	connector_rci_error_bad_value,
	connector_rci_error_COUNT
} connector_rci_error_id_t;

typedef struct connector_remote_config_data {
	struct connector_remote_group_table const * group_table;
	char const * const * error_table;
	unsigned int global_error_count;
	uint32_t firmware_target_zero_version;
	uint32_t vendor_id;
	char const * device_type;
} connector_remote_config_data_t;

#endif

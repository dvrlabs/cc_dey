/*
* Copyright (c) 2017, 2018 Digi International Inc.
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

#ifndef _CCAPI_RCI_H_
#define _CCAPI_RCI_H_

#if (defined CONST)
#undef CONST
#endif

#if (defined CCAPI_CONST_PROTECTION_UNLOCK)
#define CONST
#else
#define CONST   const
#endif

typedef enum {
    CCAPI_RCI_ELEMENT_TYPE_NOT_SET,
    CCAPI_RCI_ELEMENT_TYPE_STRING,
    CCAPI_RCI_ELEMENT_TYPE_MULTILINE_STRING,
    CCAPI_RCI_ELEMENT_TYPE_PASSWORD,
    CCAPI_RCI_ELEMENT_TYPE_INT32,
    CCAPI_RCI_ELEMENT_TYPE_UINT32,
    CCAPI_RCI_ELEMENT_TYPE_HEX32,
    CCAPI_RCI_ELEMENT_TYPE_0X32,
    CCAPI_RCI_ELEMENT_TYPE_FLOAT,
    CCAPI_RCI_ELEMENT_TYPE_ENUM,
    CCAPI_RCI_ELEMENT_TYPE_ON_OFF,
    CCAPI_RCI_ELEMENT_TYPE_BOOL,
    CCAPI_RCI_ELEMENT_TYPE_IPV4,
    CCAPI_RCI_ELEMENT_TYPE_FQDNV4,
    CCAPI_RCI_ELEMENT_TYPE_FQDNV6,
    CCAPI_RCI_ELEMENT_TYPE_LIST,
    CCAPI_RCI_ELEMENT_TYPE_MAC,
    CCAPI_RCI_ELEMENT_TYPE_DATETIME,
    CCAPI_RCI_ELEMENT_TYPE_REF_ENUM
} ccapi_rci_element_type_t;

typedef enum {
    CCAPI_OFF,
    CCAPI_ON
} ccapi_on_off_t;

typedef enum {
    CCAPI_RCI_ACTION_SET,
    CCAPI_RCI_ACTION_QUERY,
    CCAPI_RCI_ACTION_DO_COMMAND,
    CCAPI_RCI_ACTION_REBOOT,
    CCAPI_RCI_ACTION_SET_FACTORY_DEFAULTS
} ccapi_rci_action_t;

typedef enum {
    CCAPI_RCI_GROUP_SETTING,
    CCAPI_RCI_GROUP_STATE
} ccapi_rci_group_type_t;

typedef enum {
    CCAPI_RCI_COLLECTION_TYPE_FIXED_ARRAY,
    CCAPI_RCI_COLLECTION_TYPE_VARIABLE_ARRAY,
    CCAPI_RCI_COLLECTION_TYPE_FIXED_DICTIONARY,
    CCAPI_RCI_COLLECTION_TYPE_VARIABLE_DICTIONARY
} ccapi_rci_collection_type_t;

typedef enum {
    CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_CURRENT,
    CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_STORED,
    CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_SOURCE_DEFAULTS
} ccapi_rci_query_setting_attribute_source_t;

typedef enum {
    CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_NONE,
    CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_CURRENT,
    CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_STORED,
    CCAPI_RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO_DEFAULTS
} ccapi_rci_query_setting_attribute_compare_to_t;

typedef struct {
    ccapi_rci_query_setting_attribute_source_t source;
    ccapi_rci_query_setting_attribute_compare_to_t compare_to;
} ccapi_rci_query_setting_attributes_t;

typedef struct {
    unsigned int entries;
    char const * const * keys;
} ccapi_rci_dictionary_t;

typedef union {
    unsigned int index;
    char const * key;
    unsigned int count;
    ccapi_rci_dictionary_t dictionary;
} ccapi_group_item_t;

typedef union {
    unsigned int index;
    char const * key;
    unsigned int count;
    ccapi_rci_dictionary_t dictionary;
} ccapi_list_item_t;

typedef struct {
    struct {
        ccapi_rci_group_type_t CONST type;
        unsigned int CONST id;
        ccapi_rci_collection_type_t CONST collection_type;
        ccapi_group_item_t CONST item;
#if (defined RCI_PARSER_USES_COLLECTION_NAMES)
        char const * CONST name;
#endif
    } group;

#ifdef RCI_LIST_MAX_DEPTH
    struct {
        unsigned int CONST depth;
        struct {
            unsigned int CONST id;
            ccapi_rci_collection_type_t CONST collection_type;
            ccapi_list_item_t CONST item;
#if (defined RCI_PARSER_USES_COLLECTION_NAMES)
            char const * CONST name;
#endif
        } data[RCI_LIST_MAX_DEPTH];
    } list;
#endif

    struct {
        unsigned int CONST id;
#if (defined RCI_PARSER_USES_ELEMENT_NAMES)
        char const * CONST name;
#endif
        ccapi_rci_element_type_t CONST type;
    } element;

    struct {
        ccapi_rci_query_setting_attributes_t CONST attributes;
        ccapi_bool_t matches;
    } query_setting;

    struct {
        struct {
            ccapi_bool_t embed_transformed_values;
        } attributes;
    } set_setting;

    struct {
        char const * CONST target;
        char const * CONST request;
        char const * * CONST response;
    } do_command;

    ccapi_rci_action_t CONST action;
    char const * error_hint;
    void * user_context;
} ccapi_rci_info_t;

typedef union {
    char const * string_value;
    int32_t signed_integer_value;
    uint32_t unsigned_integer_value;
    float float_value;
    unsigned int enum_value;
    ccapi_on_off_t  on_off_value;
    ccapi_bool_t  boolean_value;
} ccapi_element_value_t;

typedef union {
    unsigned int count;
    ccapi_rci_dictionary_t dictionary;
} ccapi_response_item_t;

typedef unsigned int (*ccapi_rci_function_base_t)(ccapi_rci_info_t * const info);
typedef unsigned int (*ccapi_rci_function_lock_t)(ccapi_rci_info_t * const info, ccapi_response_item_t * const item);
typedef unsigned int (*ccapi_rci_function_element_t)(ccapi_rci_info_t * const info, ccapi_element_value_t * const element);
typedef unsigned int (*ccapi_rci_function_transform_t)(ccapi_rci_info_t * const info, ccapi_element_value_t * const element, char const ** const transformed);

typedef struct {
    struct {
        ccapi_rci_function_base_t start_session;
        ccapi_rci_function_base_t end_session;
        ccapi_rci_function_base_t start_action;
        ccapi_rci_function_base_t end_action;
        ccapi_rci_function_base_t start_group;
        ccapi_rci_function_base_t end_group;
        ccapi_rci_function_lock_t lock_group_instances;
        ccapi_rci_function_base_t set_group_instances;
        ccapi_rci_function_base_t remove_group_instance;
        ccapi_rci_function_base_t unlock_group_instances;
        ccapi_rci_function_base_t start_list;
        ccapi_rci_function_base_t end_list;
        ccapi_rci_function_lock_t lock_list_instances;
        ccapi_rci_function_base_t set_list_instances;
        ccapi_rci_function_base_t remove_list_instance;
        ccapi_rci_function_base_t unlock_list_instances;
        ccapi_rci_function_element_t get_element;
        ccapi_rci_function_element_t set_element;
        ccapi_rci_function_transform_t set_and_transform_element;
        ccapi_rci_function_base_t do_command;
        ccapi_rci_function_base_t set_factory_defaults;
        ccapi_rci_function_base_t reboot;
    } callback;
    struct connector_remote_config_data const * rci_desc;
} ccapi_rci_data_t;

typedef struct {
    ccapi_rci_data_t const * rci_data;
} ccapi_rci_service_t;
#endif

/*
 * Copyright (c) 2014 Digi International Inc.
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


#define GROUP_ID_VARIABLE(rci)          ((rci)->shared.group.id)
#define set_group_id(rci, value)        (GROUP_ID_VARIABLE(rci) = (value))
#define get_group_id(rci)               (GROUP_ID_VARIABLE(rci))
#define invalidate_group_id(rci)        set_group_id(rci, INVALID_ID)
#define have_group_id(rci)              (get_group_id(rci) != INVALID_ID)
#define increment_group_id(rci)         (GROUP_ID_VARIABLE(rci)++)

#define GROUP_INSTANCE_VARIABLE(rci)    ((rci)->shared.group.info.instance)
#define set_group_instance(rci, value)  (GROUP_INSTANCE_VARIABLE(rci) = (value))
#define get_group_instance(rci)         (GROUP_INSTANCE_VARIABLE(rci))
#define increment_group_instance(rci)   (GROUP_INSTANCE_VARIABLE(rci)++)
#define invalidate_group_instance(rci)  set_group_instance(rci, INVALID_INDEX)
#define have_group_instance(rci)        (get_group_instance(rci) != INVALID_INDEX)

#define GROUP_COUNT_VARIABLE(rci)       ((rci)->shared.group.info.keys.count)
#define get_group_count(rci)            (GROUP_COUNT_VARIABLE(rci))
#define set_group_count(rci, value)     (GROUP_COUNT_VARIABLE(rci) = (value))
#define have_group_count(rci)           (get_group_count(rci) != INVALID_ID)

#define GROUP_LOCK_VARIABLE(rci)        ((rci)->shared.group.lock)
#define get_group_lock(rci)             (GROUP_LOCK_VARIABLE(rci))
#define set_group_lock(rci, value)      (GROUP_LOCK_VARIABLE(rci) = (value))
#define have_group_lock(rci)            (get_group_lock(rci) != INVALID_ID)

#define GROUP_LOCK_VARIABLE(rci)        ((rci)->shared.group.lock)
#define get_group_lock(rci)             (GROUP_LOCK_VARIABLE(rci))
#define set_group_lock(rci, value)      (GROUP_LOCK_VARIABLE(rci) = (value))
#define have_group_lock(rci)            (get_group_lock(rci) != INVALID_ID)

#define get_group_name(rci) (get_group_instance(rci) == 0 ? (rci)->shared.group.info.keys.key_store : (rci)->shared.group.info.keys.list[get_group_instance(rci) - 1])

#define get_group_collection_type(rci) (get_current_group((rci))->collection.collection_type)

static connector_group_t const * get_current_group(rci_t const * const rci)
{
    connector_remote_config_data_t const * const rci_data = rci->service_data->connector_ptr->rci_data;
    connector_remote_group_table_t const * const table = rci_data->group_table + rci->shared.callback_data.group.type;
    unsigned int const group_id = get_group_id(rci);

    ASSERT(have_group_id(rci));
    ASSERT(group_id < table->count);

    return (table->groups + group_id);
}

#if (defined RCI_PARSER_USES_DICT)
static connector_bool_t group_is_dictionary(rci_t const * const rci)
{
    connector_collection_type_t collection_type = get_group_collection_type(rci);
    return is_dictionary(collection_type);
}
#endif

#if (defined RCI_PARSER_USES_VARIABLE_GROUP)
static connector_bool_t group_is_dynamic(rci_t const * const rci)
{
    connector_collection_type_t collection_type = get_group_collection_type(rci);

    return (collection_type == connector_collection_type_variable_array || collection_type == connector_collection_type_variable_dictionary) ? connector_true : connector_false;
}
#endif

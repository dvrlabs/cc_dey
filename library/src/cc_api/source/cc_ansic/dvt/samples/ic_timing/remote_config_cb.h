/*
 * Copyright (c) 2013 Digi International Inc.
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

#ifndef REMOTE_CONFIG_CB_H_
#define REMOTE_CONFIG_CB_H_

#include "connector_debug.h"

typedef connector_callback_status_t(* remote_group_cb_t) (connector_remote_config_t * const remote_config);
typedef void (* remote_group_cancel_cb_t) (connector_remote_config_cancel_t * const remote_config);

typedef struct remote_group_table {
    remote_group_cb_t init_cb;
    remote_group_cb_t set_cb;
    remote_group_cb_t get_cb;
    remote_group_cb_t end_cb;
    remote_group_cancel_cb_t cancel_cb;
} remote_group_table_t;


typedef struct {
    remote_group_table_t * group;
    void * group_context;
} remote_group_session_t;

extern connector_callback_status_t app_gps_stats_group_get(connector_remote_config_t * const remote_config);

extern connector_callback_status_t app_system_group_init(connector_remote_config_t * const remote_config);
extern connector_callback_status_t app_system_group_set(connector_remote_config_t * const remote_config);
extern connector_callback_status_t app_system_group_get(connector_remote_config_t * const remote_config);
extern connector_callback_status_t app_system_group_end(connector_remote_config_t * const remote_config);
extern void app_system_group_cancel(connector_remote_config_cancel_t * const remote_config);

#endif /* REMOTE_CONFIG_CB_H_ */

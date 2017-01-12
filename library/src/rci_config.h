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

#ifndef rci_config_h
#define rci_config_h

typedef enum {
	connector_global_error_OFFSET = connector_rci_error_COUNT,
	connector_global_error_load_fail = connector_global_error_OFFSET,
	connector_global_error_save_fail,
	connector_global_error_memory_fail,
	connector_global_error_not_implemented,
	connector_global_error_COUNT
} connector_global_error_id_t;

typedef enum {
	connector_state_device_state_system_up_time,
	connector_state_device_state_COUNT
} connector_state_device_state_id_t;

typedef enum {
	connector_state_device_state, connector_state_COUNT
} connector_state_id_t;

#endif

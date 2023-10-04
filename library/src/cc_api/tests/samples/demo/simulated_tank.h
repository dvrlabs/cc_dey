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

#ifndef _SIMULATED_TANK_H_
#define _SIMULATED_TANK_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define MAX_VOLUME      4000
#define MIN_VOLUME      0

#define INIT_VOLUME     (MAX_VOLUME / 2)
#define INIT_VALVEIN    VALVE_CLOSED
#define INIT_VALVEOUT   VALVE_OPENED
#define INIT_TEMP       35
#define VALVEOUT_FLOW   15
#define VALVEIN_FLOW    10
#define MIN_TEMP        25
#define MAX_TEMP        45

typedef enum {
  VALVE_OPENED = 0,
  VALVE_CLOSED = 1
} valve_status_t;

void update_simulated_tank_status(void);
double get_tank_temperature(void);
int32_t get_tank_fill_percentage(void);
valve_status_t get_valveIN_status(void);
void set_valveIN_status(valve_status_t new_status);
valve_status_t get_valveOUT_status(void);
void set_valveOUT_status(valve_status_t new_status);
void print_tank_status(void);

#endif

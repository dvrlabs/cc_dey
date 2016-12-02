#!/bin/sh
#===============================================================================
#
# Copyright (C) 2017 by Digi International Inc. All rights reserved.
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
#===============================================================================

DAEMON=/usr/bin/cc_dey
NAME=cc_dey
DESC="Cloud Connector daemon"
ARGS=""

test -f $DAEMON || exit 0

set -e

case "$1" in
    start)
        echo -n "starting $DESC: $NAME... "
    start-stop-daemon -S -b -n $NAME -a $DAEMON -- $ARGS
    echo "done."
    ;;
    stop)
        echo -n "stopping $DESC: $NAME... "
    start-stop-daemon -K --signal INT -n $NAME
    echo "done."
    ;;
    restart)
        echo -n "restarting $DESC: $NAME... "
    $0 stop
    $0 start
    echo "done."
    ;;
    reload)
        echo -n "reloading $DESC: $NAME... "
        killall -HUP $(basename ${DAEMON})
    echo "done."
    ;;
    *)
    echo "Usage: $0 {start|stop|restart|reload}"
    exit 1
    ;;
esac

exit 0

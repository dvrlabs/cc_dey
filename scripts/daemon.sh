#!/bin/sh
#===============================================================================
#
#  Copyright (C) 2016 by Digi International Inc. All rights reserved.
#
#  This software contains proprietary and confidential information of Digi.
#  International Inc. By accepting transfer of this copy, Recipient agrees
#  to retain this software in confidence, to prevent disclosure to others,
#  and to make no use of this software other than that for which it was
#  delivered. This is an unpublished copyrighted work of Digi International
#  Inc. Except as permitted by federal law, 17 USC 117, copying is strictly
#  prohibited.
#
#  Restricted Rights Legend
#
#  Use, duplication, or disclosure by the Government is subject to restrictions
#  set forth in sub-paragraph (c)(1)(ii) of The Rights in Technical Data and
#  Computer Software clause at DFARS 252.227-7031 or subparagraphs (c)(1) and
#  (2) of the Commercial Computer Software - Restricted Rights at 48 CFR
#  52.227-19, as applicable.
#
#  Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
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

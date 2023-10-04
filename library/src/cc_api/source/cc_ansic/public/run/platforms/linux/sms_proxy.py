#!/usr/bin/env python 
#
# ***************************************************************************
# Copyright (c) 2012 Digi International Inc.
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
# sms_proxy.py
# Proxy to test SMS functionality for the Ethrios Cloud Connector.
# -------------------------------------------------
# Usage: sms_proxy.py
# (No arguments required)
# -------------------------------------------------

import socket 
import digisms
import sys
import time
import string

#Port where to listen
port = 9999 

#Client socket
client = None

def Usage():
    print 'Usage: sms_proxy.py (No arguments required)'
    print '    Proxy to test SMS functionality for the Ethrios Cloud Connector.'
    print '    Intended to run on a Digi Gateway like ConnectPortX4,'
    print '    configured with SMS enabled but with iDigi SMS disabled.'
    print '    The application will listen on port 9999 for TCP connections from client.'
    print '    SMSs messages received by the gateway will be forwarded through TCP to the'
    print '    client.'
    print '    Messages received from the client through TCP will be sent as an SMS to the'
    print '    configured phone_number.'
    print '    To configure the phone-number send following message to the socket: '
    print '        phone-number=xxxxxxxxxx'
    print '    examples:'
    print '        for devicecloud.digi.com: phone-number=447786201216'
    print '        for test.idigi.com:  phone-number=447786201217'


def sms_callback(a):
    print """\
 
Received SMS from: %s
at: %s
====================
%s
====================
""" % (a.source_addr, a.timestamp, a.message)

    if client != None: 
        client.send(a.message) 
        # Introduce a delay to avoid two messages are sent together. TODO: flush?
        time.sleep(1)
    else:
        print ("Not connected to any client")
 

def ListenForConnections():
    global client

    backlog = 1
    size = 1024 
    
    #for tests, skip sms sent.
    mute = 0

    #device cloud telephone where to send SMSs
    #requires initialitation using 'phone-number=xxxxxxx'
    phone_number = ''

    #listen on localhost
    host = ''

    print ("mute: %d" % (mute))

    sms = digisms.Callback(sms_callback)

    exit1 = 0
    while exit1 == 0: 
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
        s.bind((host,port)) 
        s.listen(backlog) 

        client, address = s.accept() 

        # Close s to allow one single connection
        s.close()
        s = None

        print "hello ", address

        exit2 = 0
        while exit2 == 0: 
            data = client.recv(size) 
            if data == 'x': 
                exit1 = 1
                exit2 = 1
            elif data == 'm': 
                mute = not(mute)
                print ("mute: %d" % (mute))
            elif data == '': 
                # Client closed connection. Go to accept again.
                exit2 = 1
            elif data[:13] == 'phone-number=':
                phone_number = data[13:]
                print ("configuring phone-number=%s" %(phone_number))
            elif phone_number == '':
                print ("phone_number not set!!!!!!!. Use 'phone-number' command")
            elif data:
                if mute == 0:
                    print ("Sending SMS to %s:\n%s" % (phone_number, data))
                    digisms.send(phone_number, data)
                else:
                    print ("SKIP Sending SMS to %s: %s" % (phone_number, data))
            else:
                print ("XXXX: %s" % (data))

        print "bye ", address
        client.close()
        client = None
        phone_number = ''

def main(argv):
    #process arguments
    count = len(argv);
    if (count > 0):
        Usage()
    else:
        # No parameters from command line

        # Modify Battery Operated status in Device Cloud
        result = ListenForConnections()


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

# -*- coding: iso-8859-1 -*-
import cc_testcase
from string import ascii_letters, digits
import random
import time


class DeviceRequestDvtTestCase(cc_testcase.TestCase):


    def test_01_stop_connector_wait_sessions_complete_for_transport_all(self):
        """ Verifies that Connector must manage correctly stop all transports after complete all opened sessions.
            In this case should send the response to the device request correctly.
        """

        target = "test_01_stop_connector_wait_sessions_complete_for_transport_all"
        payload = "payload"

        status, response = self.cloudHandler.sendDeviceRequest(self.device_id, target, payload)

        self.log.info("Received response: '%s'" % response.content)


        if(status):
            # Status 200. Checking the received response
            try:
                responseText = response.resource["sci_reply"]["data_service"]["device"]["requests"]["device_request"]["#text"]
                if(responseText == "Request successfully processed"):
                    self.log.info("Received the expected response")

                    # Search for UDP transport stop message
                    result, line, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="ccapi_stop_transport_udp returns CCAPI_UDP_STOP_ERROR_NOT_STARTED", timeout=30)

                    if (result):
                        self.log.info("Connector UDP transport stop callback feedback received correctly")
                    else:
                        self.fail("Console feedback for UDP transport stop callback status NOT received")

                    # Search for SMS transport stop message
                    if ( dataBuffer.find("ccapi_stop_transport_sms returns CCAPI_SMS_STOP_ERROR_NOT_STARTED") != -1 ):
                        self.log.info("Connector SMS transport stop callback feedback received correctly")
                    else:
                        self.fail("Console feedback for SMS transport stop callback status NOT received")


                else:
                    self.fail("Received response from device: \"%s\" is not the expected" % responseText)
            except KeyError,e:
                self.log.exception("KeyError exception caugth: %s" % e)
                # Read all from the buffer
                self.deviceHandler.readBuffer()
                self.fail("KeyError exception caugth!!")
        else:
            self.log.error("Response content from device: %s" % response.content)
            self.fail("Incorrect response code: %d" % response.status_code)


        # Verify that the device is connected to Device Cloud
        if (self.cloudHandler.isDeviceConnected(self.device_id)):
            self.log.info("Device connected.")
            self.deviceMonitor.stayConnected(30)
            self.log.info("Device is connected after 30 seconds.")
        else:
            self.fail("Device is disconnected from Device Cloud")





    def test_02_stop_connector_wait_sessions_complete_for_transport_tcp(self):
        """ Verifies that Connector must manage correctly stop TCP transport after complete all opened sessions.
            In this case should send the response to the device request correctly.
        """

        target = "test_02_stop_connector_wait_sessions_complete_for_transport_tcp"
        payload = "payload"

        status, response = self.cloudHandler.sendDeviceRequest(self.device_id, target, payload)

        self.log.info("Received response: '%s'" % response.content)


        if(status):
            # Status 200. Checking the received response
            try:
                responseText = response.resource["sci_reply"]["data_service"]["device"]["requests"]["device_request"]["#text"]
                if(responseText == "Request successfully processed"):
                    self.log.info("Received the expected response")

                    result, line, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="connector_stop_callback: connector_transport_tcp", timeout=10)

                    if (result):
                        self.log.info("Connector TCP transport stop callback feedback received correctly")
                    else:
                        self.fail("Console feedback for TCP transport stop callback status NOT received")
                else:
                    self.fail("Received response from device: \"%s\" is not the expected" % responseText)
            except KeyError,e:
                self.log.exception("KeyError exception caugth: %s" % e)
                # Read all from the buffer
                self.deviceHandler.readBuffer()
                self.fail("KeyError exception caugth!!")
        else:
            self.log.error("Response content from device: %s" % response.content)
            self.fail("Incorrect response code: %d" % response.status_code)


        # Verify that the device disconnects from Device Cloud
        if (self.cloudHandler.isDeviceConnected(self.device_id)):
            self.log.info("Waiting for Cloud to disconnect device.")
            self.deviceMonitor.waitForDisconnect(30)
            self.log.info("Device disconnected.")
        else:
            self.log.info("Device is disconnected from Device Cloud")

        # Connector waits a few seconds to relaunch the transports on the device and we must verify that the device is connected again
        if (not self.cloudHandler.isDeviceConnected(self.device_id)):
            self.log.info("Waiting for Device to reconnect.")
            self.deviceMonitor.waitForConnect(30)
            self.log.info("Device is connected again.")



    def test_03_stop_connector_wait_sessions_complete_several_times(self):
        """ Verifies that Connector must manage correctly TCP transport after complete all opened sessions
            several times to make sure that the process is robust.
        """

        totalLoops = 25

        for i in range(0,totalLoops):
            self.log.info("")
            self.log.info("**** Loop %s of %s" % (i+1,totalLoops) )

            target = "test_02_stop_connector_wait_sessions_complete_for_transport_tcp"
            payload = "payload"

            status, response = self.cloudHandler.sendDeviceRequest(self.device_id, target, payload)

            self.log.info("Received response: '%s'" % response.content)


            if(status):
                # Status 200. Checking the received response
                try:
                    responseText = response.resource["sci_reply"]["data_service"]["device"]["requests"]["device_request"]["#text"]
                    if(responseText == "Request successfully processed"):
                        self.log.info("Received the expected response")

                        result, line, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="connector_stop_callback: connector_transport_tcp", timeout=10)

                        if (result):
                            self.log.info("Connector TCP transport stop callback feedback received correctly")
                        else:
                            self.fail("Console feedback for TCP transport stop callback status NOT received")
                    else:
                        self.fail("Received response from device: \"%s\" is not the expected" % responseText)
                except KeyError,e:
                    self.log.exception("KeyError exception caugth: %s" % e)
                    # Read all from the buffer
                    self.deviceHandler.readBuffer()
                    self.fail("KeyError exception caugth!!")
            else:
                self.log.error("Response content from device: %s" % response.content)
                self.fail("Incorrect response code: %d" % response.status_code)


            # Verify that the device disconnects from Device Cloud
            #if (self.cloudHandler.isDeviceConnected(self.device_id)):
            self.log.info("Waiting for Cloud to disconnect device.")
            self.deviceMonitor.waitForDisconnect(30)
            self.log.info("Device disconnected.")
            #else:
                #self.log.info("Device is disconnected from Device Cloud")

            # Connector waits a few seconds to relaunch the transports on the device and we must verify that the device is connected again
            if (not self.cloudHandler.isDeviceConnected(self.device_id)):
                self.log.info("Waiting for Device to reconnect.")
                self.deviceMonitor.waitForConnect(30)
                self.log.info("Device is connected again.")



    def test_04_stop_connector_stop_immediately_for_transport_tcp(self):
        """ Verifies that Connector must manage correctly stop TCP transport immediately before complete the opened sessions.
            In this case should close the transports and not send the response to the device request.
        """

        payload_size = 2048
        target = "test_04_stop_connector_stop_immediately_for_transport_tcp"
        payload = ''.join(random.choice(ascii_letters + digits) for _ in range(payload_size))

        status, response = self.cloudHandler.sendDeviceRequest(self.device_id, target, payload)

        self.log.info("Received response: '%s'" % response.content)


        if(status):
            # Status 200. Checking the received response
            try:
                responseText = response.resource["sci_reply"]["data_service"]["device"]["error"]["desc"]
                if(responseText == "Device disconnected while processing request"):
                    self.log.info("Received the expected response")

                    result, line, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="connector_stop_callback: connector_transport_tcp", timeout=10)

                    if (result):
                        self.log.info("Connector TCP transport stop callback feedback received correctly")
                    else:
                        self.fail("Console feedback for TCP transport stop callback status NOT received")
                else:
                    self.fail("Received response from device: \"%s\" is not the expected" % responseText)
            except KeyError,e:
                self.log.exception("KeyError exception caugth: %s" % e)
                # Read all from the buffer
                self.deviceHandler.readBuffer()
                self.fail("KeyError exception caugth!!")
        else:
            self.log.error("Response content from device: %s" % response.content)
            self.fail("Incorrect response code: %d" % response.status_code)


        # Verify that the device disconnects from Device Cloud
        if (self.cloudHandler.isDeviceConnected(self.device_id)):
            self.log.info("Waiting for Cloud to disconnect device.")
            self.deviceMonitor.waitForDisconnect(30)
            self.log.info("Device disconnected.")
        else:
            self.log.info("Device is disconnected from Device Cloud")

        # Connector waits a few seconds to relaunch the transports on the device and we must verify that the device is connected again
        if (not self.cloudHandler.isDeviceConnected(self.device_id)):
            self.log.info("Waiting for Device to reconnect.")
            self.deviceMonitor.waitForConnect(30)
            self.log.info("Device is connected again.")




    def test_05_stop_connector_stop_immediately_several_times(self):
        """ Verifies that Connector must manage correctly stop TCP transport immediately before complete the opened sessions
            several times to make sure that the process is robust.
        """

        totalLoops = 25

        for i in range(0,totalLoops):
            self.log.info("")
            self.log.info("**** Loop %s of %s" % (i+1,totalLoops) )

            target = "test_04_stop_connector_stop_immediately_for_transport_tcp"
            payload = "payload"

            status, response = self.cloudHandler.sendDeviceRequest(self.device_id, target, payload)

            self.log.info("Received response: '%s'" % response.content)


            if(status):
                # Status 200. Checking the received response
                try:
                    responseText = response.resource["sci_reply"]["data_service"]["device"]["error"]["desc"]
                    if(responseText == "Device disconnected while processing request"):
                        self.log.info("Received the expected response")

                        result, line, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="connector_stop_callback: connector_transport_tcp", timeout=10)

                        if (result):
                            self.log.info("Connector TCP transport stop callback feedback received correctly")
                        else:
                            self.fail("Console feedback for TCP transport stop callback status NOT received")
                    else:
                        self.fail("Received response from device: \"%s\" is not the expected" % responseText)

                except KeyError,e:
                    self.log.exception("KeyError exception caugth: %s" % e)
                    # Read all from the buffer
                    self.deviceHandler.readBuffer()
                    self.fail("KeyError exception caugth!!")
            else:
                self.log.error("Response content from device: %s" % response.content)
                self.fail("Incorrect response code: %d" % response.status_code)


            # Verify that the device disconnects from Device Cloud
            if (self.cloudHandler.isDeviceConnected(self.device_id)):
                self.log.info("Waiting for Cloud to disconnect device.")
                self.deviceMonitor.waitForDisconnect(30)
                self.log.info("Device disconnected.")
            else:
                self.log.info("Device is disconnected from Device Cloud")

            # Connector waits a few seconds to relaunch the transports on the device and we must verify that the device is connected again
            if (not self.cloudHandler.isDeviceConnected(self.device_id)):
                self.log.info("Waiting for Device to reconnect.")
                self.deviceMonitor.waitForConnect(30)
                self.log.info("Device is connected again.")



    def test_06_connector_terminate(self):
        """ Verifies that Connector must manage correctly the terminate action.
            In this case should end the connector thread and the application thread.
        """

        target = "test_06_connector_terminate"
        payload = "payload"

        status, response = self.cloudHandler.sendDeviceRequest(self.device_id, target, payload)

        self.log.info("Received response: '%s'" % response.content)


        if(status):
            # Status 200. Checking the received response
            try:
                responseText = response.resource["sci_reply"]["data_service"]["device"]["requests"]["device_request"]["#text"]
                if(responseText == "Request successfully processed"):
                #responseText = response.resource["sci_reply"]["data_service"]["device"]["error"]["desc"]
                #if(responseText == "Device disconnected while processing request"):
                    self.log.info("Received the expected response")

                    result, line, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="ccapi_stop_thread: type=2", timeout=10)

                    if (result):
                        self.log.info("Connector terminate action feedback received correctly")
                    else:
                        self.fail("Console feedback for terminate action status NOT received")
                else:
                    self.fail("Received response from device: \"%s\" is not the expected" % responseText)
            except KeyError,e:
                self.log.exception("KeyError exception caugth: %s" % e)
                # Read all from the buffer
                self.deviceHandler.readBuffer()
                self.fail("KeyError exception caugth!!")
        else:
            self.log.error("Response content from device: %s" % response.content)
            self.fail("Incorrect response code: %d" % response.status_code)


        # Verify that the device disconnects from Device Cloud
        if (self.cloudHandler.isDeviceConnected(self.device_id)):
            self.log.info("Waiting for Cloud to disconnect device.")
            self.deviceMonitor.waitForDisconnect(30)
            self.log.info("Device disconnected.")
        else:
            self.log.info("Device is disconnected from Device Cloud")

# -*- coding: iso-8859-1 -*-
import cc_testcase

class ConnectivityDvtTestCase(cc_testcase.TestCase):


    def test_01_firmware_error_download_denied(self):
        """ Sends update firmware request to force a "download denied" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_01_firmware_error_download_denied.txt'
        expectedError = "the firmware upgrade deferred by client"

        self.send_firmware_update_request_with_error( filePath, expectedError)



    def test_02_firmware_error_download_invalid_size(self):
        """ Sends update firmware request to force an "invalid size" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_02_firmware_error_download_invalid_size.txt'
        expectedError = "client has insufficient space to hold the download image"

        self.send_firmware_update_request_with_error( filePath, expectedError)



    def test_03_firmware_error_download_invalid_version(self):
        """ Sends update firmware request to force an "invalid version" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_03_firmware_error_download_invalid_version.txt'
        expectedError = "incompatible version number detected"

        self.send_firmware_update_request_with_error( filePath, expectedError)



    def test_04_firmware_error_download_unauthenticated(self):
        """ Sends update firmware request to force a "authentication" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_04_firmware_error_download_unauthenticated.txt'
        expectedError = "client has not authenticated the server"

        self.send_firmware_update_request_with_error( filePath, expectedError)



    def test_05_firmware_error_download_not_allowed(self):
        """ Sends update firmware request to force a "not allowed" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_05_firmware_error_download_not_allowed.txt'
        expectedError = "updates are  rejected by the client"

        self.send_firmware_update_request_with_error( filePath, expectedError)



    def test_06_firmware_error_download_configured_to_reject(self):
        """ Sends update firmware request to force a "rejected" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_06_firmware_error_download_configured_to_reject.txt'
        expectedError = "client does not accept upgrades"

        self.send_firmware_update_request_with_error( filePath, expectedError)



    def test_07_firmware_error_encountered_error(self):
        """ Sends update firmware request to force an "internal" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_07_firmware_error_encountered_error.txt'
        expectedError = "client error allocating space for new image or internal processing error"

        self.send_firmware_update_request_with_error( filePath, expectedError)



    def test_08_firmware_status_device_error(self):
        """ Sends update firmware request to force an "abort" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_08_firmware_status_device_error.txt'
        expectedError = "Aborted By Target"

        self.send_firmware_update_request_with_error( filePath, expectedError)



    def test_09_firmware_status_hardware_error(self):
        """ Sends update firmware request to force a "hardware" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_09_firmware_status_hardware_error.txt'
        expectedError = "Aborted By Target"
        expectedConsoleMessage = "app_firmware_image_data: test_09_firmware_status_hardware_error"

        self.send_firmware_update_request_with_error( filePath, expectedError, consoleFeedback=expectedConsoleMessage)



    def test_10_firmware_status_invalid_data(self):
        """ Sends update firmware request to force an "invalid data" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_10_firmware_status_invalid_data.txt'
        expectedError = "Aborted By Target"
        expectedConsoleMessage = "app_firmware_image_data: test_10_firmware_status_invalid_data"

        self.send_firmware_update_request_with_error( filePath, expectedError, consoleFeedback=expectedConsoleMessage)



    def test_11_firmware_status_invalid_offset(self):
        """ Sends update firmware request to force an "invalid offset" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_11_firmware_status_invalid_offset.txt'
        expectedError = "Aborted By Target"
        expectedConsoleMessage = "app_firmware_image_data: test_11_firmware_status_invalid_offset"

        self.send_firmware_update_request_with_error( filePath, expectedError, consoleFeedback=expectedConsoleMessage)



    def test_12_firmware_status_user_abort(self):
        """ Sends update firmware request to force an "user abort" error
        and verifies that the device returns the correct response.
        """

        filePath = 'test_12_firmware_status_user_abort.txt'
        expectedError = "Aborted By Target"
        expectedConsoleMessage = "app_firmware_image_data: test_12_firmware_status_user_abort"

        self.send_firmware_update_request_with_error( filePath, expectedError, consoleFeedback=expectedConsoleMessage)






    def send_firmware_update_request_with_error(self, filePath, expectedError, consoleFeedback = None):

        self.log.info("Sending firmware target query to %s." % self.device_id)
        target = 0
        fileData = "payload"
        #filePath = 'test_02_firmware_error_download_invalid_size.txt'

        # Flush previous messages
        self.deviceHandler.readBuffer()

        self.log.info("Sending request to update firmware.")
        result,response = self.cloudHandler.sendUpdateFirmwareRequest(self.device_id, fileData, fileName=filePath, target=target)
        if (result):
            # Status 200. Checking the received response
            self.fail("Unexpected success uploading firmware: %s" % response.content)
        else:
            responseText = response.resource["sci_reply"]["update_firmware"]["device"]["error"]["desc"]
            if(expectedError in responseText):
                self.log.info("Received the expected response for the update_firmware request: '%s'" % responseText)
            else:
                self.fail("Received response from device: \"%s\" is not the expected" % responseText)


        # Verify the console feedback
        if ( consoleFeedback is not None ):

            status, indexLine, dataBuffer = self.deviceHandler.readUntilPattern ( pattern=consoleFeedback, timeout=30)

            if(status):
                self.log.info("Console feedback for update firmware process")
            else:
                self.fail("Console feedback for update firmware process NOT arrived")

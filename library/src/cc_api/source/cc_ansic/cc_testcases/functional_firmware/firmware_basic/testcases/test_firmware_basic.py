# -*- coding: iso-8859-1 -*-
import cc_testcase

import random
import time
import os


MAX_TEST_FILE_SIZE = 2097152
fileContent = ""

for i in xrange(MAX_TEST_FILE_SIZE):
    fileContent += chr(random.randint(0, 255))



class ConnectivityDvtTestCase(cc_testcase.TestCase):


    def test_01_upload_valid_target_with_empty_firmware(self):
        """ Sends update firmware request with an empty file for a valid target and verifies that
        the device downloads it, disconnects and reconnects with Device Cloud server.
        """

        self.log.info("Sending firmware target query to %s." % self.device_id)
        target = 0
        fileData = ""
        filePath = os.path.join(self.tempPath, 'test_01_upload_valid_target_with_empty_firmware.txt')

        # Flush previous messages
        self.deviceHandler.readBuffer()

        self.log.info("Sending request to update firmware.")
        result,response = self.cloudHandler.sendUpdateFirmwareRequest(self.device_id, fileData, fileName=filePath, target=target)
        if (not result):
            self.fail("Unexpected error uploading firmware: %s" % response.content)

        # Verify the console feedback
        status, indexLine, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="app_firmware_reset", timeout=30)

        if(status):
            self.log.info("Console feedback for reset process")
        else:
            self.fail("Console feedback for reset process NOT arrived")


        self.log.info("Waiting for Cloud to disconnect device.")
        self.deviceMonitor.waitForDisconnect(30)
        self.log.info("Device disconnected.")

        self.log.info("Waiting for Device to reconnect.")
        self.deviceMonitor.waitForConnect(30)
        self.log.info("Device connected.")


        # Get the file from device
        if ( os.path.exists(filePath) ):
            fileHandler = open(filePath, 'rb')
            obtainedFileData = fileHandler.read()
            fileHandler.close()
        else:
            self.fail("There is not a file in path '%s'" % filePath)


        # Verify file content
        if ( fileData == obtainedFileData ):
            self.log.info("File content uploaded was successfully!")
        else:
            self.fail("Received file content from device: \"%s\"\n is not the expected: \"%s\"" % (obtainedFileData, fileData) )


        # Remove firmware file
        if ( os.path.exists(filePath) ):
            os.remove(filePath)



    def test_02_upload_invalid_target_with_empty_firmware(self):
        """ Sends update firmware request with an empty file for an invalid target
        and verifies that the device rejects it.
        """

        self.log.info("Sending firmware target query to %s." % self.device_id)
        target = 99
        fileData = ""
        filePath = os.path.join(self.tempPath, 'test_02_upload_invalid_target_with_empty_firmware.txt')

        # Flush previous messages
        self.deviceHandler.readBuffer()

        self.log.info("Sending request to update firmware.")
        result,response = self.cloudHandler.sendUpdateFirmwareRequest(self.device_id, fileData, fileName=filePath, target=target)
        if (result):
            # Status 200. Checking the received response
            self.fail("Unexpected success uploading firmware: %s" % response.content)
        else:
            responseText = response.resource["sci_reply"]["update_firmware"]["device"]["error"]["desc"]
            if(responseText == "Failed with status[11]: Invalid Target"):
                self.log.info("Received the expected response for the put_file request")
            else:
                self.fail("Received response from device: \"%s\" is not the expected" % responseText)


        # Verify the console feedback
        status, indexLine, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="fw_process: invalid target", timeout=30)

        if(status):
            self.log.info("Console feedback for invalid target")
        else:
            self.fail("Console feedback for invalid target NOT arrived")


        # Remove firmware file
        if ( os.path.exists(filePath) ):
            os.remove(filePath)



    def test_03_upload_valid_target_without_filename(self):
        """ Sends update firmware request without a filename for a valid target and verifies that
        the device downloads it, disconnects and reconnects with Device Cloud server.
        """

        self.log.info("Sending firmware target query to %s." % self.device_id)
        target = 2
        fileData = ""
        filePath = '/tmp/test_file_without_name.txt'

        # Flush previous messages
        self.deviceHandler.readBuffer()

        self.log.info("Sending request to update firmware.")
        result,response = self.cloudHandler.sendUpdateFirmwareRequest(self.device_id, fileData, target=target)
        if (not result):
            self.fail("Unexpected error uploading firmware: %s" % response.content)

        # Verify the console feedback
        status, indexLine, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="app_firmware_reset", timeout=30)

        if(status):
            self.log.info("Console feedback for reset process")
        else:
            self.fail("Console feedback for reset process NOT arrived")


        self.log.info("Waiting for Cloud to disconnect device.")
        self.deviceMonitor.waitForDisconnect(30)
        self.log.info("Device disconnected.")

        self.log.info("Waiting for Device to reconnect.")
        self.deviceMonitor.waitForConnect(30)
        self.log.info("Device connected.")


        # Verify console feedback
        if ( dataBuffer.find("Filename for the firmware update is NULL") != -1 ):
            self.log.info("Console feedback for filename empty")
        else:
            self.fail("Console feedback for filename empty NOT arrived")


        # Get the file from device
        if ( os.path.exists(filePath) ):
            fileHandler = open(filePath, 'rb')
            obtainedFileData = fileHandler.read()
            fileHandler.close()
        else:
            self.fail("There is not a file in path '%s'" % filePath)


        # Verify file content
        if ( fileData == obtainedFileData ):
            self.log.info("File content uploaded was successfully!")
        else:
            self.fail("Received file content from device: \"%s\"\n is not the expected: \"%s\"" % (obtainedFileData, fileData) )


        # Remove firmware file
        if ( os.path.exists(filePath) ):
            os.remove(filePath)




    def test_04_upload_no_target_with_valid_filename(self):
        """ Sends update firmware request without a target but with a filename based on a known pattern
        and verifies that the device downloads it, disconnects and reconnects with Device Cloud server.
        """

        self.log.info("Sending firmware target query to %s." % self.device_id)
        fileData = "payload"
        filePath = os.path.join(self.tempPath, 'test_04_upload_no_target_with_valid_filename.txt')

        # Flush previous messages
        self.deviceHandler.readBuffer()

        self.log.info("Sending request to update firmware.")
        result,response = self.cloudHandler.sendUpdateFirmwareRequest(self.device_id, fileData, fileName=filePath)
        if (not result):
            self.fail("Unexpected error uploading firmware: %s" % response.content)

        # Verify the console feedback
        status, indexLine, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="app_firmware_reset", timeout=30)

        if(status):
            self.log.info("Console feedback for reset process")
        else:
            self.fail("Console feedback for reset process NOT arrived")


        self.log.info("Waiting for Cloud to disconnect device.")
        self.deviceMonitor.waitForDisconnect(30)
        self.log.info("Device disconnected.")

        self.log.info("Waiting for Device to reconnect.")
        self.deviceMonitor.waitForConnect(30)
        self.log.info("Device connected.")


        # Get the file from device
        if ( os.path.exists(filePath) ):
            fileHandler = open(filePath, 'rb')
            obtainedFileData = fileHandler.read()
            fileHandler.close()
        else:
            self.fail("There is not a file in path '%s'" % filePath)


        # Verify file content
        if ( fileData == obtainedFileData ):
            self.log.info("File content uploaded was successfully!")
        else:
            self.fail("Received file content from device: \"%s\"\n is not the expected: \"%s\"" % (obtainedFileData, fileData) )


        # Remove firmware file
        if ( os.path.exists(filePath) ):
            os.remove(filePath)




    def test_05_upload_valid_target_several_times_with_increasing_firmware(self):
        """ Sends update firmware request several times for the same target with different firmware file
        increasing its size in each loop and verifies that the device downloads it, disconnects and reconnects with Device Cloud server.
        """

        self.log.info("Sending firmware target query to %s." % self.device_id)
        target = 0

        fileSizeList = range(0,MAX_TEST_FILE_SIZE, 102400)
        fileSizeList.append(MAX_TEST_FILE_SIZE)


        for eachFileSize in fileSizeList:
            fileData = fileContent[:eachFileSize]
            filePath = os.path.join(self.tempPath, "test_05_upload_valid_target_several_times_with_increasing_firmware_size_%s.txt" % eachFileSize)

            self.log.info("*** Sending a firmware size of %s bytes..." % eachFileSize)

            # Flush previous messages
            self.deviceHandler.readBuffer()

            self.log.info("Sending request to update firmware.")
            result,response = self.cloudHandler.sendUpdateFirmwareRequest(self.device_id, fileData, fileName=filePath, target=target)
            if (not result):
                self.fail("Unexpected error uploading firmware: %s" % response.content)

            # Verify the console feedback
            status, indexLine, dataBuffer = self.deviceHandler.readUntilPattern ( pattern="app_firmware_reset", timeout=30)

            if(status):
                self.log.info("Console feedback for reset process")
            else:
                self.fail("Console feedback for reset process NOT arrived")


            self.log.info("Waiting for Cloud to disconnect device.")
            self.deviceMonitor.waitForDisconnect(30)
            self.log.info("Device disconnected.")

            self.log.info("Waiting for Device to reconnect.")
            self.deviceMonitor.waitForConnect(30)
            self.log.info("Device connected.")


            # Get the file from device
            if ( os.path.exists(filePath) ):
                fileHandler = open(filePath, 'rb')
                obtainedFileData = fileHandler.read()
                fileHandler.close()
            else:
                self.fail("There is not a file in path '%s'" % filePath)


            # Verify file content
            if ( fileData == obtainedFileData ):
                self.log.info("File content uploaded was successfully!")
            else:
                self.fail("Received file content from device is not the expected!!!!!!")
                #self.fail("Received file content from device: \"%s\"\n is not the expected: \"%s\"" % (obtainedFileData, fileData) )


            # Remove firmware file
            if ( os.path.exists(filePath) ):
                os.remove(filePath)

            # Waiting for the next firmware update
            time.sleep(3)

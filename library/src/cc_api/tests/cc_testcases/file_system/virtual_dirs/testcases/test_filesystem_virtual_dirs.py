import cc_testcase

import os

class DiscoveryTestCase(cc_testcase.TestCase):

    def test_01_add_virtual_directory_nonexistent_path(self):
        """ Test to create a virtual directory over a nonexistent path. """

        rootPath = self.tempPath
        rootPath = os.path.join( rootPath , "test_01_virtual_dirs_folder")

        # Step 1: Send Device Request
        target = "test_01_add_virtual_directory_nonexistent_path"

        self.log.info("Send request to execute the source code for target '%s'..." % (target) )
        result, requestResponse = self.cloudHandler.sendDeviceRequest(self.device_id, target, rootPath)
        if (result):
            self.log.info("Response was successfully from device")
        else:
            self.fail("Error was found in the response: %s" % (requestResponse.content) )


        # Step 2: Looking for message
        expectedPattern = "test_01_add_virtual_directory_nonexistent_path: Error caught CCAPI_FS_ERROR_NOT_A_DIR"
        result,indexPattern,dataBuffer = self.deviceHandler.readUntilPattern ( pattern=expectedPattern, timeout=30)
        if ( not result ):
            self.fail("Console feedback was NOT arrived")
        else:
            self.log.info("Obtained expected feedback from device: '%s'" % expectedPattern)



    def test_02_add_virtual_directory_existent_path(self):
        """ Test to create a virtual directory over an existent path. """

        # Create the new directory on Device
        rootPath = self.tempPath
        rootPath = os.path.join( rootPath , "test_02_virtual_dirs_folder")
        os.makedirs(rootPath)

        # Step 1: Send Device Request
        target = "test_02_add_virtual_directory_existent_path"

        self.log.info("Send request to execute the source code for target '%s'..." % (target) )
        result, requestResponse = self.cloudHandler.sendDeviceRequest(self.device_id, target, rootPath)
        if (result):
            self.log.info("Response was successfully from device")
        else:
            self.fail("Error was found in the response: %s" % (requestResponse.content) )


        # Step 2: Looking for message
        expectedPattern = "test_02_add_virtual_directory_existent_path: Error caught CCAPI_FS_ERROR_NONE"
        result,indexPattern,dataBuffer = self.deviceHandler.readUntilPattern ( pattern=expectedPattern, timeout=30)
        if ( not result ):
            self.fail("Console feedback was NOT arrived")
        else:
            self.log.info("Obtained expected feedback from device: '%s'" % expectedPattern)



    def test_03_add_virtual_directory_already_mapped(self):
        """ Test to create a virtual directory with the same name than an existing virtual directory. """

        # Create the new directory on Device
        rootPath = self.tempPath
        rootPath = os.path.join( rootPath , "test_02_virtual_dirs_folder")

        # Step 1: Send Device Request
        target = "test_03_add_virtual_directory_already_mapped"

        self.log.info("Send request to execute the source code for target '%s'..." % (target) )
        result, requestResponse = self.cloudHandler.sendDeviceRequest(self.device_id, target, rootPath)
        if (result):
            self.log.info("Response was successfully from device")
        else:
            self.fail("Error was found in the response: %s" % (requestResponse.content) )


        # Step 2: Looking for message
        expectedPattern = "test_03_add_virtual_directory_already_mapped: Error caught CCAPI_FS_ERROR_ALREADY_MAPPED"
        result,indexPattern,dataBuffer = self.deviceHandler.readUntilPattern ( pattern=expectedPattern, timeout=30)
        if ( not result ):
            self.fail("Console feedback was NOT arrived")
        else:
            self.log.info("Obtained expected feedback from device: '%s'" % expectedPattern)

        # Remove the folder
        os.rmdir(rootPath)


    def test_04_remove_existent_virtual_directory(self):
        """ Test to remove a nonexistent virtual directory. """

        # Create the new directory on Device
        rootPath = self.tempPath
        rootPath = os.path.join( rootPath , "test_04_virtual_dirs_folder")
        os.makedirs(rootPath)

        # Step 1: Send Device Request
        target = "test_04_remove_existent_virtual_directory"

        self.log.info("Send request to execute the source code for target '%s'..." % (target) )
        result, requestResponse = self.cloudHandler.sendDeviceRequest(self.device_id, target, rootPath)
        if (result):
            self.log.info("Response was successfully from device")
        else:
            self.fail("Error was found in the response: %s" % (requestResponse.content) )


        # Step 2: Looking for message
        expectedPattern = "test_04_remove_existent_virtual_directory: Error caught CCAPI_FS_ERROR_NONE"
        result,indexPattern,dataBuffer = self.deviceHandler.readUntilPattern ( pattern=expectedPattern, timeout=30)
        if ( not result ):
            self.fail("Console feedback was NOT arrived")
        else:
            self.log.info("Obtained expected feedback from device: '%s'" % expectedPattern)

        # Remove the folder
        os.rmdir(rootPath)


    def test_05_remove_nonexistent_virtual_directory(self):
        """ Test to remove a nonexistent virtual directory. """

        # Create the new directory on Device
        rootPath = self.tempPath
        rootPath = os.path.join( rootPath , "test_05_virtual_dirs_folder")
        os.makedirs(rootPath)

        # Step 1: Send Device Request
        target = "test_05_remove_nonexistent_virtual_directory"

        self.log.info("Send request to execute the source code for target '%s'..." % (target) )
        result, requestResponse = self.cloudHandler.sendDeviceRequest(self.device_id, target, rootPath)
        if (result):
            self.log.info("Response was successfully from device")
        else:
            self.fail("Error was found in the response: %s" % (requestResponse.content) )


        # Step 2: Looking for message
        expectedPattern = "test_05_remove_nonexistent_virtual_directory: Error caught CCAPI_FS_ERROR_NOT_MAPPED"
        result,indexPattern,dataBuffer = self.deviceHandler.readUntilPattern ( pattern=expectedPattern, timeout=30)
        if ( not result ):
            self.fail("Console feedback was NOT arrived")
        else:
            self.log.info("Obtained expected feedback from device: '%s'" % expectedPattern)

        # Remove the folder
        os.rmdir(rootPath)




    def test_06_send_file_to_virtual_directory(self):
        """ Sends a put_file request to upload a file to the virtual directory path
        Verifies if the returned response is the expected and the generated file has the expected content.
        """

        # Create the new directory on Device
        rootPath = self.tempPath
        rootPath = os.path.join( rootPath , "test_06_virtual_dirs_folder")
        os.makedirs(rootPath)

        # Step 1: Send Device Request
        target = "test_06_send_file_to_virtual_directory"

        self.log.info("Send request to execute the source code for target '%s'..." % (target) )
        result, requestResponse = self.cloudHandler.sendDeviceRequest(self.device_id, target, rootPath)
        if (result):
            self.log.info("Response was successfully from device")
        else:
            self.fail("Error was found in the response: %s" % (requestResponse.content) )


        # Step 2: Looking for message
        expectedPattern = "test_06_send_file_to_virtual_directory: Error caught CCAPI_FS_ERROR_NONE"
        result,indexPattern,dataBuffer = self.deviceHandler.readUntilPattern ( pattern=expectedPattern, timeout=30)
        if ( not result ):
            self.fail("Console feedback was NOT arrived")
        else:
            self.log.info("Obtained expected feedback from device: '%s'" % expectedPattern)


        fileData = "payload"
        fileName = "test_06_send_file_to_virtual_directory.txt"

        # Send different file content
        filePath = os.path.join( "/","Test_06_virtual_path", fileName)


        # Send and verify the file
        self.uploadFileAndVerifyFileContent(filePath, fileData)

        # Remove file from device
        status, response = self.cloudHandler.removeFileFromDevice(self.device_id, filePath)
        if(status):
            self.log.info("Received the expected response for the remove request")
        else:
            self.log.error("Response content from device: %s" % response.content)
            self.fail("Incorrect response code: %d" % response.status_code)



    def test_07_remove_file_from_virtual_directory(self):
        """ Sends a rm_file request to remove a file from a virtual directory path
        Verifies if the returned response is the expected.
        """

        # Generate file path using virtual directory
        fileName = "test_06_send_file_to_virtual_directory.txt"
        filePath = os.path.join( "/","Test_06_virtual_path", fileName)

        # Remove file from device
        status, response = self.cloudHandler.removeFileFromDevice(self.device_id, filePath)
        if(status):
            self.log.info("Received the expected response for the remove request")
        else:
            self.log.error("Response content from device: %s" % response.content)
            self.fail("Incorrect response code: %d" % response.status_code)




    def uploadFileAndVerifyFileContent(self, filePath, fileData, offset = None, truncate = None, fileSize = None):

        # Send the file to the Device
        self.log.info("Uploading file '%s' with size %s to device..." % (filePath, fileSize) )
        status, response = self.cloudHandler.uploadFileToDevice(self.device_id, filePath, fileData, offset = offset, truncate = truncate, timeout = 240)

        if(status):
            # Status 200. Checking the received response
            responseText = response.resource["sci_reply"]["file_system"]["device"]["commands"]["put_file"]
            if(responseText is None):
                self.log.info("Received the expected response for the put_file request")
            else:
                self.fail("Received response from device: \"%s\" is not the expected" % responseText)
        else:
            self.log.error("Response content from device: %s" % response.content)
            self.fail("Incorrect response code: %d" % response.status_code)


        # Get the file from device
        self.log.info("Downloading file '%s' with size %s from device..." % (filePath, fileSize) )
        status, obtainedFileData, response = self.cloudHandler.downloadFileFromDevice(self.device_id, filePath, fileSize = fileSize, offset = offset, timeout = 240)

        if(status):
            self.log.info("Received the expected response for the get_file request")
        else:
            self.log.error("Response content from device: %s" % response.content)
            self.fail("Incorrect response code: %d" % response.status_code)


        # Verify file content
        if ( fileData == obtainedFileData ):
            #self.log.warning("Received file content from device: \"%s\" is the expected: \"%s\"" % (obtainedFileData, fileData) )
            self.log.info("File content received was successfully!")
        else:
            self.fail("Received file content from device: \"%s\"\n is not the expected: \"%s\"" % (obtainedFileData, fileData) )





if __name__ == '__main__':
    unittest.main()

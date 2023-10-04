import cc_testcase

class DiscoveryTestCase(cc_testcase.TestCase):

    def test_01_verify_device_type_matches(self):
        # Make sure that device is connected
        if (not self.cloudHandler.isDeviceConnected(self.device_id)):
            self.log.info("Waiting for Device to reconnect.")
            self.deviceMonitor.waitForConnect(30)
            self.log.info("Device connected.")
        self.log.info("Beginning Test to Verify Device Type.")

        self.log.info("Retrieving Device type for %s." % self.device_id)
        result, deviceType = self.cloudHandler.getDeviceType(self.device_id)
        if (result):
            self.log.info("Checking if Device Type matches %s" % self.device_type)
            self.assertEqual(deviceType, self.device_type,
                "Device type (%s) does not match device type in configuration (%s)."
                % (deviceType, self.device_type))
        else:
            self.fail("Could not get device type")


    def test_02_verify_vendor_id_matches(self):
        self.log.info("Beginning Test to Verify Vendor ID.")
        self.log.info("Retrieving Vendor ID for %s." % self.device_id)
        result, deviceVendorID = self.cloudHandler.getDeviceVendorID(self.device_id)
        if (result):
            self.log.info("Checking if Device Vendor ID matches %s" % self.vendor_id)
            self.assertEqual(deviceVendorID, self.vendor_id,
                "Vendor ID (%s) does not match Vendor ID in configuration (%s)."
                % (deviceVendorID, self.vendor_id))
        else:
            self.fail("Could not get Vendor ID")


    def test_03_verify_mac_address_matches(self):
        self.log.info("Beginning Test to Verify MAC Address.")
        self.log.info("Retrieving MAC Address for %s." % self.device_id)
        result, deviceMAC = self.cloudHandler.getDeviceMAC(self.device_id)

        if (result):
            self.log.info("Checking if Device MAC Address matches %s" % self.device_mac)
            self.assertEqual(deviceMAC.replace(":",""), self.device_mac.replace(":",""),
                "MAC Address (%s) does not match MAC Address in configuration (%s)."
                % (deviceMAC, self.device_mac))
        else:
            self.fail("Could not get MAC Address")



    def test_04_verify_firmware_level_matches(self):
        self.log.info("Beginning Test to Verify Firmware level.")
        self.log.info("Retrieving Firmware level for %s." % self.device_id)
        result, deviceFirmwareLevel = self.cloudHandler.getDeviceFirmwareLevel(self.device_id)

        if (result):
            self.log.info("Checking if Device Firmware level matches %s" % self.device_firmware)
            self.assertEqual(deviceFirmwareLevel, self.device_firmware,
                "Firmware Level (%s) does not match Firmware Level in configuration (%s)."
                % (deviceFirmwareLevel, self.device_firmware))

            self.log.info("Device Firmware level obtained '%s' match with the expected '%s'!!" % (deviceFirmwareLevel, self.device_firmware) )
        else:
            self.fail("Could not get Firmware level")


if __name__ == '__main__':
    unittest.main()

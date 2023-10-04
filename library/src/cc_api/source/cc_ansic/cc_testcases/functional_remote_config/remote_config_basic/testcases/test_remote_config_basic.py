# -*- coding: iso-8859-1 -*-
import cc_testcase




class RemoteConfigDvtTestCase(cc_testcase.TestCase):


    def test_01_disconnect(self):

        """ Sends disconnect request to given device and verifies that
        the device disconnects and reconnects with Device Cloud server,
        using the Python API to manage Device Cloud.
        """

        self.log.info("***** Beginning Disconnect Test *****")

        # Make sure that device is connected
        if (not self.cloudHandler.isDeviceConnected(self.device_id)):
            self.log.info("Waiting for Device to reconnect.")
            self.deviceMonitor.waitForConnect(30)
            self.log.info("Device connected.")

        self.log.info("Sending RCI set/get queries to %s." % self.device_id)

        groupName = "string_types"
        settingName = "test_field_string"
        settingType = "setting"
        newValue = "EEEEEEE"
        self.verifySettingValue(settingType, groupName, settingName, newValue)









    def getSettingValue(self, settingType, groupName, settingName):

        # Send RCI request
        if ( settingType == "query_setting" ):
            result,response = self.cloudHandler.getQueryDeviceSettings(self.device_id)
        elif ( settingType == "query_state" ):
            result,response = self.cloudHandler.getQueryDeviceState(self.device_id)
        else:
            self.fail("Unknown setting type '%s'" % settingType)

        if ( not result ):
            return (result,"",response)

        # Get setting value
        settingValue = response.resource["sci_reply"]["send_message"]["device"]["rci_reply"][settingType][groupName][settingName]

        # Return value for the setting
        return (True,settingValue, response)



    def setSettingValue(self, settingType, groupName, settingName, newValue, index=1):

        # Send RCI request
        if ( settingType == "set_setting" ):
            result,response = self.cloudHandler.setQueryDeviceSettings(self.device_id, groupName, index, settingName, newValue)
        elif ( settingType == "set_state" ):
            result,response = self.cloudHandler.setQueryDeviceState(self.device_id, groupName, index, settingName, newValue)
        else:
            self.fail("Unknown setting type '%s'" % settingType)

        if ( not result ):
            return (result,response)

        # Get setting value
        settingValue = response.resource["sci_reply"]["send_message"]["device"]["rci_reply"][settingType][groupName][settingName]

        # Verify response
        if( settingValue is None ):
            return (True,response)
        else:
            return (False,response)




    def verifySettingValue(self, settingType, groupName, settingName, newValue):
        # Initialize result
        result = False

        if(settingType == "setting"):
            getSettingType = "query_setting"
            setSettingType = "set_setting"
        elif(settingType == "state"):
            getSettingType = "query_state"
            setSettingType = "set_state"

        # Get original value
        self.log.info("Sending RCI query to get the setting value for '%s/%s'." % (groupName, settingName) )
        result,originalValue,xmlResponse = self.getSettingValue(getSettingType, groupName, settingName)
        self.log.info("Original value is '%s'" % (originalValue))

        # Check if we expect changes
        if(originalValue == newValue):
            self.log.warning("Original value '%s' match with the new value '%s' to be setup " % (originalValue,newValue) )

        # Set new value
        self.log.info("Sending RCI query to set the setting value for '%s/%s'." % (groupName, settingName) )
        setResult,xmlResponse = self.setSettingValue(setSettingType, groupName, settingName, newValue)
        if( setResult ):
            # Get new value
            self.log.info("Sending RCI query to get the setting value for '%s/%s'." % (groupName, settingName) )
            result,modifiedValue,xmlResponse = self.getSettingValue(getSettingType, groupName, settingName)

            # Verify original and modified values
            if(newValue == modifiedValue):
                result = True
                self.log.info("New value '%s' match with obtained value '%s'" % (newValue,modifiedValue) )
            else:
                self.log.error("New value '%s' does not match with obtained value '%s'" % (newValue,modifiedValue) )

        return result

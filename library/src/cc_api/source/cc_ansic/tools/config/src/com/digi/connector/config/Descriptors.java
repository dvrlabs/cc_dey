package com.digi.connector.config;

import java.io.BufferedReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.URL;
import java.util.Map;
import java.util.Objects;
import java.util.Base64;
import java.util.HashMap;

import javax.net.ssl.HttpsURLConnection;

import org.dom4j.io.OutputFormat;
import org.dom4j.io.XMLWriter;

public class Descriptors {

    private final String username;
    private final String password;
    private static String deviceType;
    private static Long vendorId;
    private final long fwVersion;
    private int responseCode;

    private final static Config config = Config.getInstance();
    private final static ConfigGenerator options = ConfigGenerator.getInstance();

    public Descriptors(final String username, final String password,
                       Long vendorId, final String deviceType,
                       final long version) throws IOException  {
        this.username = username;
        this.password = password;
        Descriptors.deviceType = deviceType;
        this.fwVersion = version;

        if(!options.noUploadOption())
            validateUrlName();

        if (vendorId == null) {
            vendorId = getVendorId();
            if (vendorId == null) {
                vendorId = createVendorId();
            }
        }

        if (vendorId == null || vendorId == 0) {
            throw new IOException("Invalid Vendor ID");
        }
        Descriptors.vendorId = vendorId;

        options.debug_log(String.format("Vendor ID = 0x%08X (%d)\n", vendorId, vendorId));

        this.responseCode = 0;
    }

    public void processDescriptors() throws Exception {
        final boolean uploading = !options.noUploadOption();
        Map<String, org.dom4j.Element> descriptors = new HashMap<>();

        options.log("\nProcessing Descriptors, please wait...");

        int id = 1;
        for (Group.Type type : Group.Type.values()) {
            Table table = config.getTable(type);

            /* Descriptors must be uploaded even if the group is empty */
            String config_type = type.toLowerName();

            descriptors.put("descriptor/query_" + config_type, table.generateQueryDescriptor(config, id));
            descriptors.put("descriptor/set_" + config_type, table.generateSetDescriptor(config, id + 1));
            id += 2;
        }

        if (options.rciLegacyEnabled()){
            descriptors.put("descriptor/reboot", generateRebootDescriptor());
            descriptors.put("descriptor/do_command", generateDoCommandDescriptor());
            descriptors.put("descriptor/set_factory_default", generateSetFactoryDefaultDescriptor());
        }

        descriptors.put("descriptor", generateRootDescriptor());

        if (uploading) {
           deleteDescriptors();
        }

        for (Map.Entry<String, org.dom4j.Element> entry : descriptors.entrySet()) {
            String name = entry.getKey();
            org.dom4j.Element content = entry.getValue();

            if (uploading) {
                uploadDescriptor(name, content);
            }

            if (options.saveDescriptorOption()) {
                saveDescriptor(name, content);
            }
        }
        final String done = uploading ? "uploaded" : "saved";
        options.log("\nDescriptors were " + done + " successfully.");
    }

    public void deleteDescriptors()
    {
        String target = String.format("/ws/DeviceMetaData?condition=dvVendorId=%d and dmDeviceType=\'%s\' and dmVersion=%d",
                vendorId, deviceType, fwVersion);

        String response = sendCloudData(target.replace(" ", "%20"), "DELETE", null);
        if (responseCode != 0)
        {
            options.debug_log("Response from " + options.getUrlName());
            switch (responseCode)
            {
            case 401:
            options.log("Unauthorized: verify username and password are valid\n");
            break;

            case 403:
            options.log("Forbidden: deleting previous RCI descriptors failed, verify that vendor ID is valid and is owned by your account.\n");
            break;

            default:
            options.log("Response status: " + response);
            break;
            }

            System.exit(1);
        }
        options.debug_log("Deleted target: " + target);
        options.debug_log(String.format("Deleted: 0x%X/%s", vendorId, deviceType));
        options.debug_log(response);
    }

    private org.dom4j.Element generateDoCommandDescriptor() {
        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("descriptor")
            .addAttribute("element", "do_command")
            .addAttribute("desc", "Execute command on device")
            .addAttribute("bin_id", "6");

        e.addElement("error_descriptor")
            .addAttribute("id", "1")
            .addAttribute("desc", "Invalid arguments");

        e.addElement("attr")
            .addAttribute("name", "target")
            .addAttribute("type", "string")
            .addAttribute("max", Integer.toString(config.getMaxAttributeLength()))
            .addAttribute("desc", "The subsystem that the command is forwarded to")
            .addAttribute("bin_id", "0");

        return e;
    }

    private org.dom4j.Element generateRebootDescriptor() {
        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("descriptor")
            .addAttribute("element", "reboot")
            .addAttribute("desc", "Reboot the device")
            .addAttribute("bin_id", "7");

        e.addElement("error_descriptor")
            .addAttribute("id", "1")
            .addAttribute("desc", "Reboot Failed");

        return e;
    }

    private org.dom4j.Element generateSetFactoryDefaultDescriptor() {
        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("descriptor")
            .addAttribute("element", "set_factory_default")
            .addAttribute("desc", "Set device configuration to factory defaults")
            .addAttribute("bin_id", "8");

        e.addElement("error_descriptor")
            .addAttribute("id", "1")
            .addAttribute("desc", "Set Factory Default failed");

        return e;
    }

    private void addErrorDescriptors(org.dom4j.Element e, final int start, final Map<String, String> errors) {
        Integer id = start;

        for (String value : errors.values()) {
            e.addElement("error_descriptor")
                .addAttribute("id", id.toString())
                .addAttribute("desc", Objects.toString(value, null));
            id++;
        }
    }

    private void addAvailableDescriptor(org.dom4j.Element e, final String name) {
        e.addElement("descriptor")
            .addAttribute("element", name)
            .addAttribute("dscr_avail", "true");
    }

    private org.dom4j.Element generateRootDescriptor() throws Exception {
        final String version = "1.1";
        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("descriptor");

        e.addAttribute("element", "rci_request");
        e.addAttribute("desc", "Remote Command Interface request");

        org.dom4j.Element attr = e.addElement("attr");
        attr.addAttribute("name", "version");
        attr.addAttribute("desc", "RCI version of request.  Response will be returned in this versions response format");
        attr.addAttribute("default", version);

        attr.addElement("value")
            .addAttribute("value", version)
            .addAttribute("desc", "Version " + version);

        for (Group.Type type : Group.Type.values()) {
            final int groups = config.getTable(type).size();

            if (groups != 0) {
                final String configType = type.toLowerName();

                addAvailableDescriptor(e, "query_" + configType);
                addAvailableDescriptor(e, "set_" + configType);
            }
        }

        if (options.rciLegacyEnabled()){
            addAvailableDescriptor(e, "reboot");
            addAvailableDescriptor(e, "do_command");
            addAvailableDescriptor(e, "set_factory_default");
        }

        addErrorDescriptors(e, config.getGlobalFatalProtocolErrorsOffset(), config.getGlobalFatalProtocolErrors());
        addErrorDescriptors(e, config.getGlobalProtocolErrorsOffset(), config.getGlobalProtocolErrors());

        return e;
    }

    private String sendCloudData(String target, String method, String message) {
        String response = "";
        String cloud = "https://" + options.getUrlName() + target;
        String credential = username + ":" + password;
        String encodedCredential = Base64.getEncoder().encodeToString(credential.getBytes());
        HttpsURLConnection connection = null;

        responseCode = 0;
        try {
            URL url = new URL(cloud);

            connection = (HttpsURLConnection) url.openConnection();
            connection.setRequestMethod(method);
            connection.setRequestProperty("Content-Type", "text/xml");
            connection.setRequestProperty("Authorization", "Basic " + encodedCredential);

            if (message != null) {
                connection.setDoOutput(true);

                OutputStreamWriter request = new OutputStreamWriter(connection.getOutputStream());
                request.write(message);
                request.close();
            }

            connection.connect();
            BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
            String respLine;

            while ((respLine = reader.readLine()) != null) {
                response += respLine;
            }
            reader.close();
            connection.disconnect();

        } catch (Exception resp) {
            try
            {
                responseCode = connection.getResponseCode();
                response = connection.getHeaderField(0);
            }
            catch (Exception e)
            {
                options.log("ERROR: Invalid Device Cloud URL\n");
                options.debug_log("Invalid URL: " + options.getUrlName());
                System.exit(1);
            }
        }

        return response;
    }

    private Long getVendorId() {
        Long result = null;

        options.debug_log("Query vendor ID");

        String response = sendCloudData("/ws/DeviceVendor", "GET", null);
        if (responseCode == 0) {
            int startIndex = response.indexOf("<dvVendorId>");

            if (startIndex == -1 || startIndex != response.lastIndexOf("<dvVendorId>")) {
                options.debug_log("No vendor ID found.");

            } else {
                startIndex += "<dvVendorId>".length();
                result =  Long.parseUnsignedLong(response.substring(startIndex, response.indexOf("</dvVendorId>")));
            }
        } else {
            options.debug_log("Response from " + options.getUrlName());
            switch (responseCode) {
            case 401:
                options.log("Unauthorized: verify username and password are valid\n");
                break;

            case 403:
                options.log("Forbidden: verify that your account has the \'Embedded Device Customization\' service subscribed.\n");
                break;

            default:
                options.log("Response status: " + response);
                break;
            }
            System.exit(1);
        }

        return result;
    }

    private Long createVendorId() {
        Long result = null;

        options.debug_log("Create a new vendor ID");
        sendCloudData("/ws/DeviceVendor", "POST", "<DeviceVendor></DeviceVendor>");

        String response = sendCloudData("/ws/DeviceVendor", "GET", null);
        if (responseCode != 0) {
            options.log("Response status: " + response);
        }

        int startIndex = response.indexOf("<dvVendorId>");
        if (startIndex == -1) {
            options.log(
                "Cannot create a new vendor ID for "
                + username
                + "user. User needs to manually create one. Refer to \"Setup your Device Cloud Acount\" section of the Getting started guide to obtain one.");
            System.exit(1);
        }

        if (startIndex != response.lastIndexOf("<dvVendorId>")) {
            options.log(username + " has more than one vendor ID, so please specify the correct one.");
            System.exit(1);
        }

        startIndex += "<dvVendorId>".length();
        result = Long.parseUnsignedLong(response.substring(startIndex, response.indexOf("</dvVendorId>")));

        if (result != null)
            options.log(String.format("Device Cloud registered vendor ID: 0x%X", vendorId));

        return result;
    }


    private void validateUrlName() {
        options.debug_log("Start validating device cloud url " + options.getUrlName());
        String response = sendCloudData("/ws/UserInfo", "GET", null);

        if (responseCode != 0)
        {
            options.debug_log("Response from " + options.getUrlName());
            switch (responseCode)
            {
                case 401:
                    options.log("Unauthorized: verify username and password are valid\n");
                    break;

                case 403:
                    options.log("Forbidden: Failed to get user info.\n");
                    break;

                default:
                    options.log("Response status: " + response);
                    break;
            }

            System.exit(1);
        }
    }

    private org.dom4j.Element commonMetadata(final String name) {
        org.dom4j.Element md = org.dom4j.DocumentHelper.createElement("DeviceMetaData");

        md.addElement("dvVendorId").addText(String.format("0x%08X", vendorId));
        md.addElement("dmDeviceType").addText(deviceType);
        md.addElement("dmVersion").addText(String.format("%d", fwVersion));
        md.addElement("dmName").addText(name);

        return md;
    }

    private void saveDescriptor(String name, org.dom4j.Element descriptor) {
        options.debug_log("Saving descriptor:" + name);

        org.dom4j.Element md = commonMetadata(name);
        md.addElement("dmData").add(descriptor);

        try {
            final String path = name.replace("/", "_") + ".xml";
            OutputFormat format = new OutputFormat();
            XMLWriter writer = new XMLWriter(new FileWriter(path), format);

            writer.write(md);
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void uploadDescriptor(String name, org.dom4j.Element descriptor) {
        options.debug_log("Uploading descriptor:" + name);

        org.dom4j.Element md = commonMetadata(name);
           md.addElement("dmData").addText(descriptor.asXML());

        String response = sendCloudData("/ws/DeviceMetaData", "POST", md.asXML());
        if (responseCode != 0)
        {
            options.debug_log("Response from " + options.getUrlName());
            switch (responseCode)
            {
                case 401:
                    options.log("Unauthorized: verify username and password are valid\n");
                    break;

                case 403:
                    options.log("Forbidden: Uploading " + name + " failed, verify that vendor ID is valid and is owned by your account.\n");
                    break;

                default:
                    options.log("Response status: " + response);
                    break;
            }

            System.exit(1);
        }

        options.debug_log("Created: " + vendorId + "/" + deviceType + "/" + name);
        options.debug_log(response);
        /* prevent error HTTP/1.1 429 Too Many Requests */
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public static Long vendorId() {
        return vendorId;
    }

    public static String deviceType() {
        return deviceType;
    }
}

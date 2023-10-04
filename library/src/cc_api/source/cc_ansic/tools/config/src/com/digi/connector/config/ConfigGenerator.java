package com.digi.connector.config;

import java.io.Console;
import java.io.File;
import java.io.IOException;
import java.util.Scanner;
import java.util.EnumSet;
import java.util.LinkedList;
import java.util.Set;
import java.util.Collections;

public class ConfigGenerator {
    private static ConfigGenerator instance = null;
    private ConfigGenerator(String args[]) {
        int argCount = 0;
        String argLog = "";

        for (String arg : args) {
            if (arg.startsWith(DASH)) {
                String str = arg.substring(DASH.length());

                toOption(str);

                argLog += arg + " ";
            } else {
                toArgument(++argCount, arg);
            }
        }
        argumentLog = "\"" + argLog + "\"";

        if (argCount != 4 && fileTypeOption() != FileType.GLOBAL_HEADER) {
            log("Missing arguments!");
            usage();
        }
        if (password == null && fileTypeOption() != FileType.GLOBAL_HEADER) {
            queryPassword();
        }

        instance = this;
    }
    public static final ConfigGenerator getInstance() { assert instance != null; return instance; }

// 1.0.0.0 version XML RCI
// 2.0.0.0 version Binary RCI only
// 3.0.0.0 version List support & specific callbacks moved to user space

    public final static String VERSION = "3.0.0.0";

    public static enum ItemType { COLLECTIONS, ELEMENTS, VALUES };
    public static enum FileType { NONE, SOURCE, GLOBAL_HEADER;
        public static FileType toFileType(String str) throws Exception {
            try {
                return valueOf(str.toUpperCase());
            } catch (Exception e) {
                throw new Exception("Invalid file type: " + str);
            }
        }
    }

    private final static long FIRMWARE_VERSION_MAX_VALUE = 4294967295L;

    private final static String NO_DESC_OPTION = "nodesc";
    private final static String HELP_OPTION = "help";
    private final static String VERBOSE_OPTION = "verbose";
    private final static String VENDOR_OPTION = "vendor";
    private final static String DIRECTORY_OPTION = "path";
    private final static String DELETE_DESCRIPTOR_OPTION = "deleteDescriptor";
    private final static String CCAPI_OPTION = "ccapi";
    private final static String C99_OPTION = "c99";
    private final static String CCAPI_STUB_FUNCTIONS_OPTION = "ccapiStub";
    private final static String SAVE_DESCRIPTORS_OPTION = "saveDescriptors";
    private final static String NO_UPLOAD_OPTION = "noUpload";
    private final static String RCI_LEGACY_COMMANDS_OPTION = "rci_legacy_commands";
    private final static String RCI_DC_TARGET_MAX_OPTION = "rci_dc_attribute_max_len";
    private final static String NO_BACKUP_OPTION = "noBackup";
    private final static String RCI_PARSER_OPTION = "rci_parser";
    private final static String OVERRIDE_MAX_NAME_LENGTH = "maxNameLength";
    private final static String OVERRIDE_MAX_KEY_LENGTH = "maxKeyLength";
    private final static String FILE_TYPE_OPTION = "type";

    private final static String URL_OPTION = "url";
    private final static String URL_DEFAULT = "devicecloud.digi.com";

    private final static String USE_NAMES_OPTION = "usenames";
    private final static String USE_NAMES_DEFAULT = "none";
    
    private final static String USE_ENUMS_OPTION = "useenums";
    private final static String USE_ENUMS_DEFAULT = "all";

    private final static String PREFIX_OPTION = "prefix";

    private final static String DASH = "-";

    private final static String USERNAME = "username";
    private final static String PASSWORD = "password";
    private final static String DEVICE_TYPE = "deviceType";
    private final static String FIRMWARE_VERSION = "firmwareVersion";
    private final static String CONFIG_FILENAME = "configFileName";

    private String urlName = URL_DEFAULT;
    private Long vendorId;

    private String deviceType;
    private long fwVersion;
    private String username;
    private String password;
    private String filename;
    private String argumentLog;
    private String directory = ".";
    private boolean noErrorDescription;
    private boolean verboseOption;
    private FileType fileType = FileType.NONE;
    private EnumSet<ItemType> useNames = EnumSet.noneOf(ItemType.class);
    private EnumSet<ItemType> useEnums = EnumSet.allOf(ItemType.class);
    private String customPrefix = "";
    private boolean deleteDescriptor;
    private boolean useCcapi;
    private boolean c99;
    private boolean CcapiStubFunctions;
    private boolean saveDescriptor;
    private boolean noUpload;
    private boolean rci_legacy;
    private Integer rci_dc_attribute_max_len;
    private Integer rci_dc_max_key_len;
    private Integer rci_dc_max_name_len;
    private boolean noBackup;
    private boolean rciParser;

    private void usage() {
        Config config = Config.getInstance();
        String className = ConfigGenerator.class.getName();

        int firstChar = className.lastIndexOf(".") + 1;
        if (firstChar != -1) {
            className = className.substring(firstChar);
        }

        log(String.format("\nUsage: java -jar %s.jar [", className)
                + DASH
                + HELP_OPTION
                + "] ["
                + DASH
                + VERBOSE_OPTION
                + "] ["
                + DASH
                + NO_DESC_OPTION
                + "] ["
                + DASH
                + VENDOR_OPTION
                + "] ["
                + DASH
                + DIRECTORY_OPTION
                + "] ["
                + DASH
                + URL_OPTION
                + "] ["
                + DASH
                + NO_BACKUP_OPTION
                + "] ["
                + DASH
                + SAVE_DESCRIPTORS_OPTION
                + "] ["
                + DASH
                + NO_UPLOAD_OPTION
                + "] ["
                + DASH
                + USE_NAMES_OPTION
                +"] ["
                + DASH
                + OVERRIDE_MAX_NAME_LENGTH
                +"] ["
                + DASH
                + OVERRIDE_MAX_KEY_LENGTH
                +"] "
                + String.format("<\"%s\"[:\"%s\"]> <%s> <%s> <%s>\n", USERNAME,
                        PASSWORD, DEVICE_TYPE, FIRMWARE_VERSION,
                        CONFIG_FILENAME));

        log("Description:");
        log("\tIt generates and uploads configuration information (descriptors) to Device Cloud");
        log("\tand it also generates ANSI C API header file (connector_api_remote.h) ");
        log("\tfrom the input configuration file.\n");

        log("Options:");
        log(String.format("\t%-16s \t= show this message", DASH
                + HELP_OPTION));
        log(String
                .format(
                        "\t%-16s \t= optional output messages about what the tool is doing",
                        DASH + VERBOSE_OPTION));
        log(String
                .format(
                        "\t%-16s \t= optional exclusion of error descriptions in the C file (Code size reduction)",
                        DASH + NO_DESC_OPTION));
        log(String
                .format(
                        "\t%-16s \t= optional vendor ID obtained from Device Cloud registration.",
                        DASH + VENDOR_OPTION + "=<vendorID>"));
        log(String
                .format(
                        "\t%-16s \t  If not given, tool tries to retrieve it from Device Cloud",
                        ""));

        log(String
                .format(
                        "\t%-16s \t= optional directory path where the header file will be created.",
                        DASH + DIRECTORY_OPTION + "=<directory path>"));
        log(String
                .format(
                        "\t%-16s \t= optional Device Cloud URL. Default is %s",
                        DASH + URL_OPTION + "=<Device Cloud URL>", URL_DEFAULT));
        log(String
                .format(
                        "\t%-16s \t= do not backup remote_config.c and .h files.",
                        DASH + NO_BACKUP_OPTION));
        log(String
                .format(
                        "\t%-16s \t= save a local copy of the Descriptors that the tool uploads to Device Cloud",
                        DASH + SAVE_DESCRIPTORS_OPTION));
        log(String
                .format(
                        "\t%-16s \t= do not upload Descriptors to Device Cloud.If this option and -vendor are set,you don't" +
                        " need to use a valid username:password.",
                        DASH + NO_UPLOAD_OPTION));
        log(String
                .format(
                        "\t%-16s \t= optional, add ASCIIZ string \"name\" field to connector_remote_group_t and/or connector_remote_element_t structures. Default is %s.",
                        DASH + USE_NAMES_OPTION + "={none|collections|elements|values|all}", USE_NAMES_DEFAULT));
        log(String
                .format(
                        "\t%-16s \t= optional, generate ids as enums for specified item types. Default is %s.",
                        DASH + USE_ENUMS_OPTION + "={none|collections|elements|values|all}", USE_ENUMS_DEFAULT));
        log(String
                .format(
                        "\n\t%-16s \t= username to log in to Device Cloud. If no password is given you will be prompted to enter the password",
                        USERNAME));
        log(String
                .format(
                        "\t%-16s \t= optional password to log in to Device Cloud",
                        PASSWORD));
        log(String
                .format(
                        "\t%-16s \t= Device type string with quotes(i.e. \"device type\")",
                        DEVICE_TYPE));
        log(String.format("\t%-16s \t= firmware version (i.e. 1.0.0.0)",
                FIRMWARE_VERSION));
        log(String.format("\t%-16s \t= The Connector Configuration file",
                CONFIG_FILENAME));
        log(String
                .format(
                        "\t%-16s \t= output for use with CCAPI projects",
                        DASH + CCAPI_OPTION));
        log(String
                .format(
                        "\t%-16s \t= use c99 syntax",
                        DASH + C99_OPTION));
        log(String
            .format(
                    "\t%-16s \t= delete current Descriptors in Device Cloud",
                    DASH + DELETE_DESCRIPTOR_OPTION));
        log(String
            .format(
                    "\t%-16s \t= optional behavior,adding a prefix to the structures.",
                    DASH + PREFIX_OPTION + "=<prefix>"));
        log(String
                .format(
                        "\t%-16s \t= Choose the output Type. Default is none.",
                        DASH + FILE_TYPE_OPTION + "={none|source|global_header}" ));
        log(String
                .format(
                        "\t%-16s \t= optional support for RCI do_command,reboot and set_factory_default",
                        DASH + RCI_LEGACY_COMMANDS_OPTION));
        log(String
                .format(
                        "\t%-16s \t= optional max length for commands attributes type string, default is %d",
                        DASH + RCI_DC_TARGET_MAX_OPTION, config.getMaxAttributeLength())); // TODO: Move this setting to the ConfigGenerator class
        log(String
                .format(
                        "\t%-16s \t= generate stub functions for CCAPI projects (assumes %s)",
                        DASH + CCAPI_STUB_FUNCTIONS_OPTION, DASH + CCAPI_OPTION));
        log(String
                .format(
                        "\t%-16s \t= Defines and enums names to work with RCI Parser.",
                        DASH + RCI_PARSER_OPTION));
        log(String
                .format(
                        "\t%-16s \t= optional behavior, defining the max length of element names.",
                        DASH + OVERRIDE_MAX_NAME_LENGTH + "=<integer>"));
        log(String
                .format(
                        "\t%-16s \t= optional behavior, defining the max length of key names.",
                        DASH + OVERRIDE_MAX_KEY_LENGTH + "=<integer>"));

        System.exit(1);
    }

    private void queryPassword() {

      Console console = System.console();

      if (console == null) {
          System.out.println("Couldn't get Console instance, maybe you're running this from within an IDE?");
          System.exit(1);
      }

      char passwordArray[] = console.readPassword("Enter password: ");

      if (passwordArray.length == 0) {
          log("You must enter a password.\nPlease try again!");
          System.exit(1);
      }
      password = new String(passwordArray);

    }

    private EnumSet<ItemType> parseItemTypeList(String option) throws Exception {
        EnumSet<ItemType> result;
        String compact = option.replace(" ",  "").toUpperCase();

        switch (compact) {
        case "NONE":
            result = EnumSet.noneOf(ItemType.class);
            break;

        case "ALL":
            result = EnumSet.allOf(ItemType.class);
            break;

        default:
            result = EnumSet.noneOf(ItemType.class);
            for (String type: compact.split(",")) {
                try {
                    result.add(ItemType.valueOf(type));
                } catch (Exception e) {
                    log("Available item types:" + ItemType.values());
                    throw new Exception("Invalid item type: " + type);
                }
            }
        }

        return result;
    }

    private Integer parsePositiveInteger(final String value, final String option) throws IOException {
        try {
            Integer result = Integer.parseUnsignedInt(value);
            if (result == 0)
                throw new NumberFormatException();

            return result;
        } catch (NumberFormatException e) {
            throw new IOException(option + " expected an positive integer value");
        }
    }

    private void toOption(String option) {

        /* split the [option]=[option value] */
        try {

            String[] keys = option.split("=", 2);

            if (keys.length == 2) {
                if (keys[0].equals(URL_OPTION)) {
                    urlName = keys[1];
                } else if (keys[0].equals(VENDOR_OPTION)) {
                    try {
                        final boolean hex = keys[1].startsWith("0x");
                        final int radix = hex ? 16 : 10;
                        final String parse = hex ? keys[1].substring(2) : keys[1];

                        vendorId = Long.parseUnsignedLong(parse, radix);
                    } catch (NumberFormatException e) {
                        throw new Exception("Invalid format for Vendor ID");
                    }
                } else if (keys[0].equals(DIRECTORY_OPTION)) {
                    if (new File(keys[1]).isDirectory()) {
                        directory = keys[1];
                    } else {
                        throw new Exception("Invalid directory path!");
                    }
                } else if (keys[0].equals(FILE_TYPE_OPTION)) {
                    fileType = FileType.toFileType(keys[1]);
                } else if (keys[0].equals(PREFIX_OPTION)) {
                    customPrefix = keys[1] + "_";
                } else if (keys[0].equals(USE_NAMES_OPTION)) {
                    useNames = parseItemTypeList(keys[1]);
                } else if (keys[0].equals(USE_ENUMS_OPTION)) {
                    useEnums = parseItemTypeList(keys[1]);
                } else if (keys[0].equals(RCI_DC_TARGET_MAX_OPTION)) {
                    rci_dc_attribute_max_len = parsePositiveInteger(keys[1], "-rci_dc_attribute_max_len");
                } else if (keys[0].equals(OVERRIDE_MAX_NAME_LENGTH)) {
                    rci_dc_max_name_len = parsePositiveInteger(keys[1], "-maxNameLength");
                } else if (keys[0].equals(OVERRIDE_MAX_KEY_LENGTH)) {
                    rci_dc_max_key_len = parsePositiveInteger(keys[1], "-maxKeyLength");
                } else {
                    throw new Exception("Invalid Option: " + keys[0]);
                }
            } else if (option.equals(NO_DESC_OPTION)) {
                noErrorDescription = true;
            } else if (option.equals(VERBOSE_OPTION)) {
                verboseOption = true;
            } else if (option.equals(DELETE_DESCRIPTOR_OPTION)) {
                deleteDescriptor = true;
            } else if (option.equals(HELP_OPTION)) {
                usage();
            } else if (option.equals(CCAPI_OPTION)) {
                useCcapi = true;
            } else if (option.equals(C99_OPTION)) {
                c99 = true;
            } else if (option.equals(CCAPI_STUB_FUNCTIONS_OPTION)) {
                useCcapi = true;
                CcapiStubFunctions = true;
            } else if (option.equals(SAVE_DESCRIPTORS_OPTION)) {
                saveDescriptor = true;
            } else if (option.equals(NO_UPLOAD_OPTION)) {
                noUpload = true;
            } else if (option.equals(RCI_LEGACY_COMMANDS_OPTION)) {
                rci_legacy = true;
            } else if (option.equals(NO_BACKUP_OPTION)) {
                noBackup = true;
            } else if (option.equals(RCI_PARSER_OPTION)) {
                rciParser = true;
            } else if (option.isEmpty()) {
                throw new Exception("Missing Option!");
            } else {
                throw new Exception("Invalid Option: " + option);
            }
        } catch (Exception e) {
            log(e.getMessage());
            usage();
            System.exit(1);
        }
    }

    private boolean getDottedFwVersion(String arg) {

        String[] versions = arg.split("\\.");
        int length = versions.length;

        if (length == 0 || length > 4) return false;

        for (String ver : versions)
        {
            int vnumber;
            try {
                vnumber = Integer.parseInt(ver);
                if (vnumber > 255) {
                    /* let's make > max fw version and return true
                     * for exceeded error msg.
                     */
                    fwVersion = FIRMWARE_VERSION_MAX_VALUE + 1;
                    break;
                }
            } catch (Exception e) {
                return false;
            }
            length--;
            fwVersion += ((vnumber << (8 * length)) & FIRMWARE_VERSION_MAX_VALUE);
        }

        return true;
    }

    private void toArgument(int argNumber, String arg) {
        try {
            switch (argNumber) {
            case 1:
                /* username:password argument */
                if (arg.indexOf(':') != -1) {
                    String[] userpass = arg.split(":", 2);

                    username = userpass[0];
                    argumentLog += username + ":";

                    if (!userpass[1].isEmpty()) {
                        password = userpass[1];
                        argumentLog += "*****";
                    }
                } else {
                    username = arg;
                    argumentLog += username;
                }

                break;
            case 2:
                /* device type */
                deviceType = arg;
                argumentLog += " \"" + arg + "\"";
                break;
            case 3:
                /* firmware version */

                Scanner fwVersionScan = new Scanner(arg);

                /* see whether it's decimal firmware version number */
                if (!fwVersionScan.hasNextLong()) {
                    /* check hex number */
                    if (arg.startsWith("0x"))
                    {
                        try {
                            fwVersion = Integer.parseInt(arg.substring(2), 16);
                        } catch (Exception e) {
                            throw new Exception("Invalid F/W Version (non-hexadecimal): " + arg);
                        }
                    }
                    /* check dotted format */
                    else if (!getDottedFwVersion(arg))
                    {
                        throw new Exception("Invalid F/W Version (non-digit or exceed maximum number of digits): " + arg);
                    }
                } else {
                    fwVersion = fwVersionScan.nextLong();
                }

                if (fwVersion > FIRMWARE_VERSION_MAX_VALUE) {
                    throw new Exception(String.format("Exceeded maximum firmware version number %s > %d (0x%X, or %d.%d.%d.%d)", arg,
                                                      FIRMWARE_VERSION_MAX_VALUE, FIRMWARE_VERSION_MAX_VALUE,
                                                      ((FIRMWARE_VERSION_MAX_VALUE >> 24) & 0xFF),
                                                      ((FIRMWARE_VERSION_MAX_VALUE >> 16) & 0xFF),
                                                      ((FIRMWARE_VERSION_MAX_VALUE >> 8) & 0xFF),
                                                      (FIRMWARE_VERSION_MAX_VALUE & 0xFF)));
                }
                debug_log(String.format("FW version: %s = %d (0x%X)", arg, fwVersion,fwVersion));

                argumentLog += " " + arg;
                break;

            case 4:
                filename = arg;
                argumentLog += " " + arg;

                break;

            default:
                log("Unkown argument: " + arg);
                return;
            }
        } catch (Exception e) {
            log(e.getMessage());
            System.exit(1);
        }
    }

    public Long getVendorId() {
        return vendorId;
    }

    public String getDeviceType() {
        return deviceType;
    }

    public long getFirmware() {
        return fwVersion;
    }

    public String getCustomPrefix() {
        return customPrefix;
    }

    public String getArgumentLogString() {
        return argumentLog;
    }

    public String getUrlName() {
        return urlName;
    }

    public boolean excludeErrorDescription() {
        return noErrorDescription;
    }

    public boolean useCcapi() {
        return useCcapi;
    }

    public boolean c99() {
        return c99;
    }

    public boolean generateCcapiStubFunctions() {
        return CcapiStubFunctions;
    }

    public boolean rciLegacyEnabled() {
        return rci_legacy;
    }

    public void log(Object aObject) {
        System.out.println(String.valueOf(aObject));
    }

    public void debug_log(Object aObject) {
        if (verboseOption) {
            System.out.println(String.valueOf(aObject));
        }
    }

    public FileType fileTypeOption() {
        if (useCcapi())
        {
            fileType = FileType.SOURCE;
        }
        return fileType;
    }

    public Set<ItemType> useNames() {
        return Collections.unmodifiableSet(useNames);
    }
    
    public Set<ItemType> useEnums() {
        return Collections.unmodifiableSet(useEnums);
    }

    public boolean deleteDescriptorOption() {
        return deleteDescriptor;
    }

    public boolean saveDescriptorOption() {
        return saveDescriptor;
    }

    public boolean noUploadOption() {
        return noUpload;
    }

    public boolean noBackupOption() {
        return noBackup;
    }

    public boolean rciParserOption() {
        return rciParser;
    }

    public String getDir() {
        return directory;
    }

    public String filename() {
        return filename;
    }

    private final void execute() throws Exception {
        Config config = Config.getInstance();

        if (deleteDescriptorOption()) {
            /* descriptor constructor for arguments */
            Descriptors descriptors = new Descriptors(username, password,
                    vendorId, deviceType, fwVersion);

            /* Generate and upload descriptors */
            debug_log("Start deleting descriptors");
            descriptors.deleteDescriptors();
            log("\nDescriptors deleted.\n");
            System.exit(0);
        }

        /* parse file */
        debug_log("Start reading filename: " + filename);

        if (rci_legacy){
            debug_log("rci legacy commands enabled");
        }

        config.setMaxAttributeLength(rci_dc_attribute_max_len);
        config.setMaxDynamicKeyLength(rci_dc_max_key_len);
        config.setMaxNameLength(rci_dc_max_name_len);

        if (fileTypeOption() != FileType.GLOBAL_HEADER) {
            Parser.processFile(filename);

            int setting_groups = config.getTable(Group.Type.SETTING).size();
            int state_groups = config.getTable(Group.Type.STATE).size();
            int total_groups = setting_groups + state_groups;
            if (total_groups == 0)
                throw new IOException("No groups specified in file: " + filename);

            debug_log("Number of setting groups: " + setting_groups);
            debug_log("Number of state groups: " + state_groups);
        }

        /* Generate H and C files */
        debug_log("Start generating files");
        LinkedList<GenFile> files = new LinkedList<>();

        switch(fileTypeOption())
        {
        case NONE:
            files.add(new GenFsmHeaderFile(config.getTypesSeen(), config.isVariableGroupSeen(), config.isVariableListSeen(), config.isDictionarySeen(), config.isVariableArraySeen(), config.isVariableDictSeen()));
            files.add(new GenFsmSourceFile());
            break;

        case SOURCE:

            if (useCcapi())
            {
                   files.add(new GenFsmFullHeaderFile());
                files.add(new GenFsmSourceFile());

                   GenApiUserHeaderFile header = new GenApiUserHeaderFile();
                files.add(header);
                files.add(new GenApiUserSourceFile(header));

                if (generateCcapiStubFunctions()) {
                    files.add(new GenApiUserStubsFile(header));
                }
            } else {
                files.add(new GenFsmUserHeaderFile());
                files.add(new GenFsmUserSourceFile());
            }
            break;

        case GLOBAL_HEADER:
            files.add(new GenFsmFullHeaderFile());
            break;
        }

        if (!useNames().isEmpty()) {
            files.add(new GenUserNamesHeaderFile());
        }

        if (!noUpload || saveDescriptor) {
            debug_log("Start Generating/uploading descriptors");

            Descriptors descriptors = new Descriptors(username, password, vendorId, deviceType, fwVersion);
            // If the user requested the vendorID from Device Cloud, store it for future use.
            if (vendorId == null) {
                vendorId = Descriptors.vendorId();
            }
            descriptors.processDescriptors();
        }

        //generate
        for (GenFile file: files) {
            file.generateFile();
        }

        log("Done.");
    }

    public static void main(String[] args) {
        try {
            instance = new ConfigGenerator(args);

            instance.execute();
            System.exit(0);

        } catch (Exception e) {
            if (e.getMessage() != null) {
                instance.log(e.getMessage());
            }

            if (e.getCause() != null)
                System.err.println(e.getCause());

            e.printStackTrace();
            System.exit(1);
        }
    }
}

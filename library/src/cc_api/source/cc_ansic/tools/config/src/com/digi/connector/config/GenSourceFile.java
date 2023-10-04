package com.digi.connector.config;

import java.io.IOException;

public abstract class GenSourceFile extends GenFile {
    protected final static String HEADER_FILENAME = "connector_api_remote.h";

    protected final static String CCAPI_PREFIX = "ccapi";

    protected final static String DEFINE = "#define ";
    protected final static String INCLUDE = "#include ";
    protected final static String ERROR = "error";

    protected final static String CONNECTOR_GLOBAL_HEADER = "\n\n#include \"connector_api.h\"\n\n";

    protected final static String TYPEDEF_ENUM = "typedef enum {\n";

    protected final static String CONNECTOR_REMOTE_ALL_STRING = "connector_remote_all_strings";
    protected final static String CONNECTOR_REMOTE_GROUP_TABLE = "connector_group_table";

    protected final static String COUNT_STRING = "COUNT";
    protected final static String OFFSET_STRING = "OFFSET";

    protected final static String CHAR_CONST_STRING = "static char CONST * CONST ";
    protected final static String CONNECTOR_CALLBACK_STATUS = "\nconnector_callback_status_t ";
    protected final static String ID_T_STRING = "_id_t;\n\n";
    protected final static String TYPEDEF_STRUCT = "\ntypedef struct {\n";

    /* Do not change these (if you do, you also need to update connector_remote.h */
    protected final static String RCI_PARSER_USES = "RCI_PARSER_USES_";
    protected final static String RCI_INFO_T = "ccapi_rci_info_t * const info";
    protected final static String RCI_FUNCTION_T = "(ccapi_rci_function_t)";
    protected final static String CCAPI_RCI_FUNCTION_T = "(ccapi_rci_function_t)";

    protected final static String RCI_PARSER_USES_ERROR_DESCRIPTIONS = RCI_PARSER_USES + "ERROR_DESCRIPTIONS\n";
    protected final static String RCI_PARSER_USES_STRINGS = RCI_PARSER_USES + "STRINGS\n";
    protected final static String RCI_PARSER_USES_UNSIGNED_INTEGER = RCI_PARSER_USES + "UNSIGNED_INTEGER\n";

    protected final static String RCI_PARSER_DATA = "CONNECTOR_RCI_PARSER_INTERNAL_DATA";

    protected final static String RCI_LEGACY_DEFINE = "\n#define RCI_LEGACY_COMMANDS\n";
    protected final static String RCI_PARSER_DEFINE = "\n#define RCI_ENUMS_AS_STRINGS\n";
    protected final static String RCI_ERROR_NOT_AVAILABLE = "connector_rci_error_not_available = -1,\n";

    /* Following enum has to be in syncr. with sendDescriptors() function */
    protected final static String RCI_QUERY_COMMAND_ATTRIBUTE_ID_T = "\ntypedef enum {\n" +
    "  rci_query_setting_attribute_id_source,\n" +      /* 'source' attribute is bin_id=0 in the uploaded descriptor for query command */
    "  rci_query_setting_attribute_id_compare_to,\n" +  /* 'compare_to' attribute is bin_id=1 in the uploaded descriptor for query command */
    "  rci_query_setting_attribute_id_count\n" +
    "} rci_query_setting_attribute_id_t;\n";

    protected final static String CONNECTOR_REMOTE_GROUP_TYPE = "\ntypedef enum {\n" +
    "    connector_remote_group_setting,\n" +
    "    connector_remote_group_state\n" +
    "} connector_remote_group_type_t;\n";

    protected final static String CONNECTOR_ELEMENT_ACCESS = "\ntypedef enum {\n" +
    "    connector_element_access_read_only,\n" +
    "    connector_element_access_write_only,\n" +
    "    connector_element_access_read_write\n" +
    "} connector_element_access_t;\n";

    protected final static String RCI_QUERY_SETTING_ATTRIBUTE_SOURCE = "\ntypedef enum {\n" +
    "    rci_query_setting_attribute_source_current,\n" +
    "    rci_query_setting_attribute_source_stored,\n" +
    "    rci_query_setting_attribute_source_defaults\n" +
    "} rci_query_setting_attribute_source_t;\n";

    protected final static String RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO = "\ntypedef enum {\n" +
    "    rci_query_setting_attribute_compare_to_none,\n" +
    "    rci_query_setting_attribute_compare_to_current,\n" +
    "    rci_query_setting_attribute_compare_to_stored,\n" +
    "    rci_query_setting_attribute_compare_to_defaults\n" +
    "} rci_query_setting_attribute_compare_to_t;\n";

    protected final static String CONNECTOR_REMOTE_CONFIG_CANCEL_T = "\ntypedef struct {\n" +
    "  void * user_context;\n" +
    "} connector_remote_config_cancel_t;\n";

    protected final static String CONNECTOR_REMOTE_GROUP_TABLE_T = "\ntypedef struct connector_remote_group_table {\n" +
    "  connector_group_t CONST * groups;\n" +
    "  size_t count;\n" +
    "} connector_remote_group_table_t;\n\n";

    protected String filePath = "";
    protected String generatedFile = "";

    protected String configType;

    public GenSourceFile(String file, GenFile.Type type, GenFile.UsePrefix use) throws IOException {
        super(file, type, use);
    }
}

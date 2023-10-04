package com.digi.connector.config;

import java.io.IOException;
import java.util.Collection;
import java.util.EnumSet;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.LinkedList;

import com.digi.connector.config.ConfigGenerator.ItemType;

public class GenFsmHeaderFile extends GenHeaderFile {

    private final static String FILENAME = "connector_api_remote.h";

    private EnumSet<Element.Type> types;
    private boolean variableGroupSeen;
    private boolean variableListSeen;
    private boolean dictionarySeen;
    private boolean variableArraySeen;
    private boolean variableDictSeen;

    public GenFsmHeaderFile(EnumSet<Element.Type> validTypes, boolean variableGroupSeen, boolean variableListSeen, boolean dictionarySeen, boolean variableArraySeen, boolean variableDictSeen) throws IOException {
        super(FILENAME, GenFile.Type.USER, GenFile.UsePrefix.NONE);
        this.variableGroupSeen = variableGroupSeen;
        this.variableListSeen = variableListSeen;
        this.dictionarySeen = dictionarySeen;
        this.variableArraySeen = variableArraySeen;
        this.variableDictSeen = variableDictSeen;
        types = validTypes;
    }

    public void writeGuardedContent() throws Exception {
        writeDefinesAndStructures();

        writeErrorHeader(GLOBAL_FATAL_PROTOCOL_ERROR, config.getGlobalFatalProtocolErrorsOffset(), config.getGlobalFatalProtocolErrors());
        writeErrorHeader(GLOBAL_PROTOCOL_ERROR, config.getGlobalProtocolErrorsOffset(), config.getGlobalProtocolErrors());
        writeErrorHeader(GLOBAL_ERROR, config.getGlobalUserErrorsOffset(), config.getGlobalUserErrors());

        writeGroupTypeAndErrorEnum();

        {
            LinkedList<String> fields = new LinkedList<>();

            fields.add(Code.Type.struct("connector_remote_group_table").constant().pointer().named("group_table"));
            fields.add(CHAR.constant().pointer().constant().pointer().named("error_table"));
            fields.add(UINT.named("global_error_count"));
            fields.add(UINT32.named("firmware_target_zero_version"));
            fields.add(UINT32.named("vendor_id"));
            fields.add(CHAR.constant().pointer().named("device_type"));

            writeBlock(Code.structTaggedTypedef("connector_remote_config_data", fields, "connector_remote_config_data_t"));
        }

        write("\nextern connector_remote_config_data_t const * const rci_descriptor_data;\n\n");

        write(
            "\n" +
            "#if !defined _CONNECTOR_API_H\n" +
            "#error \"Illegal inclusion of connector_api_remote.h. You should only include connector_api.h in user code.\"\n" +
            "#endif\n" +
            "\n");

    }

    private void writeDefineOptionHeader() throws IOException {
        LinkedList<String> defines = new LinkedList<>();

        if (!options.excludeErrorDescription()) {
            defines.add(Code.define(RCI_PARSER_USES + "ERROR_DESCRIPTIONS"));
        }

        for (Element.Type type : types) {
            defines.add(Code.define(RCI_PARSER_USES + type.toUpperName()));
        }

        EnumSet<Element.Type> isUnsigned = EnumSet.of(Element.Type.UINT32, Element.Type.HEX32, Element.Type.X_HEX32);
        for (Element.Type type : types) {
            if (isUnsigned.contains(type)) {
                defines.add(Code.define(RCI_PARSER_USES + "UNSIGNED_INTEGER"));
                break;
            }
        }

        EnumSet<Element.Type> isString = EnumSet.of(
            Element.Type.STRING, Element.Type.MULTILINE_STRING, Element.Type.PASSWORD,
            Element.Type.IPV4, Element.Type.FQDNV4, Element.Type.FQDNV6, Element.Type.MAC_ADDR,
            Element.Type.DATETIME, Element.Type.REF_ENUM);
        for (Element.Type type : types) {
            if (isString.contains(type)) {
                defines.add(Code.define(RCI_PARSER_USES + "STRINGS"));
                break;
            }
        }

        if (variableGroupSeen) {
            defines.add(Code.define(RCI_PARSER_USES + "VARIABLE_GROUP"));
        }
        if (variableListSeen) {
            defines.add(Code.define(RCI_PARSER_USES + "VARIABLE_LIST"));
        }
        if (dictionarySeen) {
            defines.add(Code.define(RCI_PARSER_USES + "DICT"));
        }
        if (variableArraySeen) {
            defines.add(Code.define(RCI_PARSER_USES + "VARIABLE_ARRAY"));
        }
        if (variableDictSeen) {
            defines.add(Code.define(RCI_PARSER_USES + "VARIABLE_DICT"));
        }

        writeBlock(defines);

        if (types.contains(Element.Type.FLOAT)) {
            writeBlock(Code.include("float.h"));
        }
    }

    private void writeElementTypeEnums() throws IOException {
        LinkedList<String> enum_lines = new LinkedList<>();
        int previous = -1;
        for (Element.Type type : types) {
            String enum_line = "    connector_element_type_" + type.toLowerName();

            int current = type.toValue();
            if (current != (previous + 1)) {
                enum_line += String.format(" = %d", current);
            }
            previous = current;

            enum_lines.add(enum_line);
        }

        assert(enum_lines.size() != 0);
        write(
            "\n" +
            "\n" +
            "typedef enum {\n" +
            String.join(",\n", enum_lines) + "\n" +
            "} connector_element_value_type_t;\n");
    }

    private String enumStructureString() {
        String string = TYPEDEF_STRUCT
                      + "    size_t count;\n";
        string += "} connector_element_value_enum_t;\n";

        return string;
    }

    private void writeElementValueStruct() throws IOException {

        String headerString = "";
        String structString = "";
        String elementValueStruct = "";
        String defineElementString = "";

        int optionCount = 0;

        Boolean isUnsignedIntegerDefined = false;
        Boolean isStringDefined = false;
        Boolean isEnumValueStructDefined = false;

        for (Element.Type type : types) {
            switch (type) {
            case UINT32:
            case HEX32:
            case X_HEX32:
                if (!isUnsignedIntegerDefined) {
                    /* if not defined yet, then define it */
                    structString += TYPEDEF_STRUCT
                        + "   uint32_t min_value;\n"
                        + "   uint32_t max_value;\n"
                        + "} connector_element_value_unsigned_integer_t;\n";
                    elementValueStruct += "    uint32_t unsigned_integer_value;\n";
                    if(options.rciParserOption())
                        defineElementString += "#define UNSIGNED_INTEGER_VALUE\n";
                    isUnsignedIntegerDefined = true;
                    optionCount++;
                }
                break;

            case INT32:
                structString += TYPEDEF_STRUCT
                                + "   int32_t min_value;\n"
                                + "   int32_t max_value;\n"
                                + "} connector_element_value_signed_integer_t;\n";
                elementValueStruct += "    int32_t signed_integer_value;\n";
                if(options.rciParserOption())
                    defineElementString += "#define SIGNED_INTEGER_VALUE\n";
                optionCount++;
                break;

            case ENUM:
                if (!isEnumValueStructDefined) {
                    structString += enumStructureString();
                    isEnumValueStructDefined = true;
                }
                elementValueStruct += "    unsigned int enum_value;\n";
                if(options.rciParserOption())
                    defineElementString += "#define ENUM_VALUE\n";
                optionCount++;
                break;

            case FLOAT:
                structString += TYPEDEF_STRUCT
                                + "    float min_value;\n"
                                + "    float max_value;\n"
                                + "} connector_element_value_float_t;\n";
                elementValueStruct += "    float float_value;\n";
                if(options.rciParserOption())
                    defineElementString += "#define FLOAT_VALUE\n";
                optionCount++;
                break;

            case ON_OFF:
                if (!isEnumValueStructDefined) {
                    /* rci parser needs this structure for on/off type */
                    structString += enumStructureString();
                    isEnumValueStructDefined = true;
                }
                elementValueStruct += "    connector_on_off_t  on_off_value;\n";
                if(options.rciParserOption())
                    defineElementString += "#define ON_OFF_VALUE\n";
                optionCount++;
                break;

            case BOOLEAN:
                if (!isEnumValueStructDefined) {
                    /* rci parser needs this structure for boolean type */
                    structString += enumStructureString();
                    isEnumValueStructDefined = true;
                }
                elementValueStruct += "    connector_bool_t  boolean_value;\n";
                if(options.rciParserOption())
                    defineElementString += "#define BOOLEAN_VALUE\n";
                optionCount++;
                break;

            default:
                if (!isStringDefined) {
                    /* if not defined yet then define it */
                    structString += TYPEDEF_STRUCT
                                    + "    size_t min_length_in_bytes;\n"
                                    + "    size_t max_length_in_bytes;\n"
                                    + "} connector_element_value_string_t;\n";
                    elementValueStruct += "    char const * string_value;\n";
                    if(options.rciParserOption())
                        defineElementString += "#define STRING_VALUE\n";
                    isStringDefined = true;
                    optionCount++;
                }
                break;
            }
        }

        if(options.rciLegacyEnabled() && !isStringDefined) {
            /* if not defined yet then define it */
            structString += TYPEDEF_STRUCT
                            + "    size_t min_length_in_bytes;\n"
                            + "    size_t max_length_in_bytes;\n"
                            + "} connector_element_value_string_t;\n";
            elementValueStruct += "    char const * string_value;\n";
            isStringDefined = true;
            optionCount++;
        }

        headerString += structString;

        if (optionCount > 1) {
            headerString += "\n\ntypedef union {\n";
        } else {
            headerString += "\n\ntypedef struct {\n";
        }

        headerString += elementValueStruct + "} connector_element_value_t;\n";

        if(options.rciParserOption())
            headerString += defineElementString;
        write(headerString);
    }

    /* returns field name (or "" if not used) */
    private String writeGroupElementDefine(Config config, ItemType type) throws IOException {
        String field = "";

        if (options.useNames().contains(type)) {
            String value = type.name();
            boolean plural = value.endsWith("S");
            String name = plural ? value.substring(0, value.length() - 1) : value;

            switch (options.fileTypeOption())
            {
                case SOURCE:
                case GLOBAL_HEADER:
                    break;

                case NONE:
                    write(String.format("\n" + "#define RCI_%s_NAME_MAX_SIZE %d",
                        name, config.getMaxNameLengthSeen(type) + 1));
            }
            field = String.format("    char const * name;\n", name);
        }

        return field;
    }

    private void writeGroupElementStructs() throws IOException {
        String element_name_struct_field = writeGroupElementDefine(config, ItemType.ELEMENTS);
        String collection_name_struct_field = writeGroupElementDefine(config, ItemType.COLLECTIONS);
        String dictionary_struct_field = dictionarySeen ? "    connector_dictionary_t dictionary;\n" : "";

        write("\n");

        if (options.rciParserOption() || options.useNames().contains(ItemType.VALUES)) {
            write(
                "\ntypedef struct {\n" +
                "    char const * const name;\n" +
                "} connector_element_enum_t;\n"
                );
        }

        String element_enum_data = "";
        if (options.rciParserOption() || options.useNames().contains(ItemType.VALUES)) {
            element_enum_data =
                "    struct {\n"+
                "        size_t count;\n"+
                "        connector_element_enum_t CONST * CONST data;\n"+
                "    } enums;\n";
        }

        write(
                   "\n" +
                "typedef struct {\n" +
                element_name_struct_field +
                "    connector_element_value_t const * const default_value;\n" +
                "    connector_element_access_t access;\n" +
                   element_enum_data +
                "} connector_element_t;\n"
                );

        if (dictionarySeen) {
            write(
                    "\n" +
                    "typedef struct {\n" +
                    "    unsigned int entries;\n" +
                    "    char const * const * keys;\n" +
                    "} connector_dictionary_t;\n"
                    );
        }

        write(
                   "\n" +
                "typedef union {\n" +
                "    size_t instances;\n" +
                dictionary_struct_field +
                "} connector_collection_capacity_t;\n"
                );

        write(
                   "\n" +
                "typedef struct {\n" +
                collection_name_struct_field +
                "    connector_collection_type_t collection_type;\n" +
                "    connector_collection_capacity_t capacity;\n" +
                "    struct {\n" +
                "        size_t count;\n" +
                "        struct connector_item CONST * CONST data;\n" +
                "    } item;\n" +
                "} connector_collection_t;\n"
                );

        write(
            "\n" +
            "typedef union {\n"
            );
        if (types.contains(Element.Type.LIST)) {
            write("    connector_collection_t CONST * CONST collection;\n");
        }
        write(
            "    connector_element_t CONST * CONST element;\n" +
            "} connector_item_data_t;\n"
            );

        write(
            "\n" +
            "typedef struct connector_item {\n" +
            "    connector_element_value_type_t type;\n" +
            "    connector_item_data_t data;\n" +
            "} connector_item_t;\n"
            );

        write(
                "\ntypedef struct {\n" +
                "    connector_collection_t collection;\n" +
                "    struct {\n" +
                "        size_t count;\n" +
                "        char CONST * CONST * description;\n" +
                "    } errors;\n" +
                "} connector_group_t;\n" +
                "\n"
                );
    }

    private void writeDefinesAndStructures() throws IOException {
        boolean haveLists = types.contains(Element.Type.LIST);
        String optional_field;

        writeDefineOptionHeader();

        if (options.rciLegacyEnabled() || options.useCcapi()){
            write(RCI_LEGACY_DEFINE);
        }
        if (options.rciParserOption() || options.useNames().contains(ItemType.VALUES)) {
            write(RCI_PARSER_DEFINE);
        }

        write(String.format("%sRCI_COMMANDS_ATTRIBUTE_MAX_LEN %d\n", DEFINE, config.getMaxAttributeLength()));
        if (haveLists) {
            // TODO: Currently the CCAPI layer requires all values to be defined even if they are not in the RCI configuration file.
            // Unfortunately, this means that if no lists are defined the value RCI_LIST_MAX_DEPTH is set to zero.
            // Because both the CCFSM and CCAPI layers are currently not conditionally compiled a value of zero is causing compiler errors.
            // For now a value of zero will be written as a value of one.
            // The thinking is that the correct fix for this issue will be to use a unified collection array and the removal of the distinction
            // between groups and lists. - ASK & PO
            int max_depth = config.getMaxDepth();
            write(String.format("%sRCI_LIST_MAX_DEPTH %d\n", DEFINE, max_depth == 0 ? 1 : max_depth));
            write(String.format("%sRCI_DICT_MAX_KEY_LENGTH %d\n", DEFINE, config.getMaxDynamicKeyLength()));
        }

        if (types.contains(Element.Type.ON_OFF)) {
            write(
                "\n" +
                "typedef enum {\n" +
                "    connector_off,\n" +
                "    connector_on\n" +
                "} connector_on_off_t;\n");
        }

        writeElementTypeEnums();
        writeElementValueStruct();

        String group_instances_lock = "";
        String group_instances_set = "";
        String group_instance_remove = "";
        String group_instances_unlock = "";

        String list_instances_lock = "";
        String list_instances_set = "";
        String list_instance_remove = "";
        String list_start = "";
        String list_end = "";
        String list_instances_unlock = "";

        String variable_array_type = "";
        String fixed_dictionary_type = "";
        String variable_dictionary_type = "";

        String dictionary_struct_field = "";
        String key_struct_field = "";

        if (haveLists) {
            if (variableListSeen) {
                list_instances_lock      = "    connector_request_id_remote_config_list_instances_lock,\n";
                list_instances_set       = "    connector_request_id_remote_config_list_instances_set,\n";
                list_instance_remove     = "    connector_request_id_remote_config_list_instance_remove,\n";
                list_instances_unlock    = "    connector_request_id_remote_config_list_instances_unlock,\n";
            }
            list_start                   = "    connector_request_id_remote_config_list_start,\n";
            list_end                     = "    connector_request_id_remote_config_list_end,\n";
        }

        if (variableGroupSeen) {
            group_instances_lock         = "    connector_request_id_remote_config_group_instances_lock,\n";
            group_instances_set          = "    connector_request_id_remote_config_group_instances_set,\n";
            group_instance_remove        = "    connector_request_id_remote_config_group_instance_remove,\n";
            group_instances_unlock       = "    connector_request_id_remote_config_group_instances_unlock,\n";
        }

        if (variableArraySeen) {
            variable_array_type          = ",\n    connector_collection_type_variable_array";
        }
        if (dictionarySeen) {
            dictionary_struct_field      = "    connector_dictionary_t dictionary;\n";
            key_struct_field             = "    char const * key;\n";
            fixed_dictionary_type        = ",\n    connector_collection_type_fixed_dictionary";
            if (variableDictSeen) {
                variable_dictionary_type = ",\n    connector_collection_type_variable_dictionary";
            }
        }

        write("\ntypedef enum {\n" +
                         "    connector_request_id_remote_config_session_start,\n" +
                         "    connector_request_id_remote_config_action_start,\n" +
                         group_instances_lock +
                         group_instances_set +
                         group_instance_remove +
                         "    connector_request_id_remote_config_group_start,\n" +
                         list_instances_lock +
                         list_instances_set +
                         list_instance_remove +
                         list_start +
                         "    connector_request_id_remote_config_element_process,\n" +
                         list_end +
                         list_instances_unlock +
                         "    connector_request_id_remote_config_group_end,\n" +
                         group_instances_unlock +
                         "    connector_request_id_remote_config_action_end,\n" +
                         "    connector_request_id_remote_config_session_end,\n" +
                         "    connector_request_id_remote_config_session_cancel");

        if (options.rciLegacyEnabled() || options.useCcapi()){
            write(",\n    connector_request_id_remote_config_do_command,\n" +
                             "    connector_request_id_remote_config_reboot,\n" +
                             "    connector_request_id_remote_config_set_factory_def");
        }
        write("\n} connector_request_id_remote_config_t;\n");

        write(
            "\n" +
            "/* deprecated */\n" +
            "#define connector_request_id_remote_config_group_process connector_request_id_remote_config_element_process\n"
            );

        write("\ntypedef enum {\n" +
                         "    connector_remote_action_set,\n" +
                         "    connector_remote_action_query");
        if (options.rciLegacyEnabled() || options.useCcapi()){
            write(",\n    connector_remote_action_do_command,\n" +
                             "    connector_remote_action_reboot,\n" +
                             "    connector_remote_action_set_factory_def");
        }
        write("\n} connector_remote_action_t;\n");

        write(CONNECTOR_REMOTE_GROUP_TYPE);

        write(CONNECTOR_ELEMENT_ACCESS);

        write(
            "\n" +
            "typedef enum {\n" +
            "    connector_collection_type_fixed_array" +
            variable_array_type +
            fixed_dictionary_type +
            variable_dictionary_type +
            "\n} connector_collection_type_t;\n"
            );

        writeGroupElementStructs();

        if (options.useNames().contains(ItemType.COLLECTIONS)) {
            write(
                "\n" +
                DEFINE + "RCI_PARSER_USES_COLLECTION_NAMES\n"
                );
        }

        write(
            "\n" +
            "typedef union {\n" +
            "    unsigned int index;\n" +
            key_struct_field +
            "    unsigned int count;\n" +
            dictionary_struct_field +
            "} connector_group_item_t;\n"
            );

        optional_field = options.useNames().contains(ItemType.COLLECTIONS)
            ? "    char const * CONST name;\n"
            : "";

        write(
            "\n" +
            "typedef struct {\n" +
            "    connector_remote_group_type_t type;\n" +
            "    unsigned int id;\n" +
            "    connector_collection_type_t collection_type;\n" +
            "    connector_group_item_t item;\n" +
            optional_field +
            "} connector_remote_group_t;\n"
            );

        optional_field = options.useNames().contains(ItemType.ELEMENTS)
            ? "    char const * CONST name;\n"
            : "";
        if (options.useNames().contains(ItemType.ELEMENTS)) {
            write("\n" + DEFINE + "RCI_PARSER_USES_ELEMENT_NAMES\n");
        }
        write(
            "\n" +
            "typedef struct {\n" +
            "    unsigned int id;\n" +
            "    connector_element_value_type_t type;\n" +
            "    connector_element_value_t * value;\n" +
            optional_field +
            "} connector_remote_element_t;\n"
            );

        write(RCI_QUERY_SETTING_ATTRIBUTE_SOURCE);
        write(RCI_QUERY_SETTING_ATTRIBUTE_COMPARE_TO);

        write("\ntypedef struct {\n" +
                         "  rci_query_setting_attribute_source_t source;\n" +
                         "  rci_query_setting_attribute_compare_to_t compare_to;\n" +
                         "  connector_bool_t embed_transformed_values;\n");
        if (options.rciLegacyEnabled() || options.useCcapi()){
            write("  char const * target;\n");
        }
        write("} connector_remote_attribute_t;\n");

        write(RCI_QUERY_COMMAND_ATTRIBUTE_ID_T);
        write(RCI_SET_COMMAND_ATTRIBUTE_ID_T);

        if (haveLists) {
            write(
                   "\n" +
                   "typedef union {\n" +
                "    unsigned int index;\n" +
                "    char const * key;\n" +
                "    unsigned int count;\n" +
                "    connector_dictionary_t dictionary;\n" +
                "} connector_list_item_t;\n"
                );

            optional_field = options.useNames().contains(ItemType.COLLECTIONS)
                ? "        char const * CONST name;\n"
                : "";

            write(
                   "\n" +
                   "typedef struct {\n" +
                "    unsigned int depth;\n" +
                "    struct {\n" +
                "        unsigned int id;\n" +
                "        connector_collection_type_t collection_type;\n" +
                "        connector_list_item_t item;\n" +
                optional_field +
                "    } level[RCI_LIST_MAX_DEPTH];\n" +
                "} connector_remote_list_t;\n"
                );
        }

        write(
               "\n" +
               "typedef union {\n" +
            "    unsigned int count;\n" +
            dictionary_struct_field +
            "} connector_response_item_t;\n"
            );

        optional_field = haveLists
            ? "    connector_remote_list_t CONST list;\n"
            : "";
        write(
               "\n" +
               "typedef struct {\n" +
            "    void * user_context;\n" +
            "    connector_remote_action_t CONST action;\n" +
            "    connector_remote_attribute_t CONST attribute;\n" +
            "    connector_remote_group_t CONST group;\n" +
            optional_field +
            "    connector_remote_element_t CONST element;\n" +
            "    unsigned int error_id;\n" +
            "\n" +
            "    struct {\n" +
            "        connector_bool_t compare_matches;\n" +
            "        char const * error_hint;\n" +
            "        connector_element_value_t * element_value;\n" +
            "        connector_response_item_t item;\n" +
            "    } response;\n" +
            "} connector_remote_config_t;\n"
            );

        write(CONNECTOR_REMOTE_CONFIG_CANCEL_T);
        write(CONNECTOR_REMOTE_GROUP_TABLE_T);

    }

    private String COMMENTED(String comment) {
        return "/* " + comment + " */";
    }

    private void writeErrorHeader(String enumDefine, Map<String, String> errors, boolean first) throws IOException {
        for (String key : errors.keySet()) {
            String error_string = enumDefine + "_" + key;

            if (first) {
                error_string += " = 1";
                first = false;
            }
            error_string += ",\n";
            write(error_string);
        }
    }

    private void writeErrorHeader(final String enumDefine, final int offset, final Map<String, String> errors) throws IOException {
        final int first = offset;
        final int count = errors.keySet().size();
        final int last = first + count - 1;

        if (count == 0) {
            return;
        }

        write(
            "typedef enum {\n"
        );

        int current = first;
        for (String key : errors.keySet()) {
            String error_string = " " + enumDefine + "_" + key;

            if (current == first) {
                error_string += " = " + offset;
            }

            if (current == last) {
                error_string += "\n";
            } else {
                error_string += ",\n";
            }

            write(error_string);
            current++;
        }

        write(
            "} " + customPrefix + enumDefine + "_id_t;\n" +
            "#define " + enumDefine + "_FIRST " + first + "\n" +
            "#define " + enumDefine + "_LAST " + last + "\n" +
            "#define " + enumDefine + "_COUNT " + count + "\n" +
            "\n"
        );
    }

    private String endEnumString(String group_name) {
        /*Add _COUNT */
        String str = " " + customPrefix + CONNECTOR_PREFIX + "_" + configType;
        if(group_name!=null)
           str += "_" + group_name;
        str += "_" + COUNT_STRING +"\n";

        str += "} " + customPrefix + CONNECTOR_PREFIX + "_" + configType;
        if(group_name!=null)
            str += "_" + group_name;
        str += ID_T_STRING;

        return str;
    }

    private void writeElementEnums(ItemList list, String prefix) throws Exception {
        String element_enum_string = TYPEDEF_ENUM;

        for (Item item : list.getItems()) {
            String item_prefix = prefix + "_" + item.getSanitizedName();

            assert (item instanceof Element) || (item instanceof ItemList);

            if (item instanceof Element) {
                element_enum_string += getEnumString(item_prefix) + ",\n";
            } else {
                ItemList sublist = (ItemList) item;

                writeElementEnums(sublist, item_prefix + "_");
            }
        }
        element_enum_string += endEnumString(prefix);

        write(element_enum_string);
    }

    private void writeValueEnums(ItemList list, String prefix) throws Exception {
        for (Item item : list.getItems()) {
            String item_prefix = prefix + "_" + item.getSanitizedName();

            assert (item instanceof Element) || (item instanceof ItemList);

            if (item instanceof Element) {
                Element element = (Element) item;

                if (element.getType() != Element.Type.ENUM)
                    continue;

                boolean explicit = false;
                int index = 0;

                write(TYPEDEF_ENUM);
                for (Value value : element.getValues()) {
                    if (value.getName().equals(""))
                        explicit = true;
                    else {
                        String line = getEnumString(item_prefix + "_" + value.getSanitizedName());

                        if (explicit) {
                            line += " = " + index;
                            explicit = false;
                        }
                        write(line + ",\n");
                    }
                    index++;
                }
                write(endEnumString(item_prefix));
            } else {
                ItemList sublist = (ItemList) item;

                writeValueEnums(sublist, item_prefix + "_");
            }
        }
    }

    private String getEnumString(String enum_name) {
        String str = " " + customPrefix + CONNECTOR_PREFIX + "_" + configType;

        if (enum_name != null) {
            str += "_" + enum_name;
        }
        return str;
    }

    private void writeAllEnumHeaders(Config config, Collection<Group> groups) throws Exception {

        for (Group group : groups) {
            if (options.useEnums().contains(ItemType.ELEMENTS)) {
                writeElementEnums(group, group.getSanitizedName());
            }
            if (options.useEnums().contains(ItemType.VALUES)) {
                writeValueEnums(group, group.getSanitizedName());
            }

            if (!group.getErrors().isEmpty()) {
                write(TYPEDEF_ENUM);

                writeErrorHeader(getEnumString(group.getSanitizedName() + "_fatal_protocol_error"), config.getGlobalFatalProtocolErrors(), true);
                writeErrorHeader(getEnumString(group.getSanitizedName() + "_protocol_error"), config.getGlobalProtocolErrors(), false);
                writeErrorHeader(getEnumString(group.getSanitizedName() + "_global_error"), config.getGlobalUserErrors(), false);

                LinkedHashMap<String, String> errorMap = group.getErrors();
                int index = 0;

                for (String key : errorMap.keySet()) {
                    String enumString = getEnumString(group.getSanitizedName() + "_" + "error" + "_" + key);

                    if (index++ == 0) {
                        /*Set start index to the global count */
                        enumString += ", " + COMMENTED("User defined (group errors)") + "\n";
                    }
                    else{
                    enumString += ",\n";
                    }

                    write(enumString);
                }
                write(endEnumString(group.getSanitizedName() + "_" + "error"));
            }
        }
    }

    private void writeGroupTypeAndErrorEnum() throws Exception {

        for (Group.Type type : Group.Type.values()) {
            Collection<Group> groups = config.getTable(type).groups();

            configType = type.toLowerName();

            if (!groups.isEmpty()) {
                writeAllEnumHeaders(config, groups);

                if (options.useEnums().contains(ItemType.COLLECTIONS)) {
                    String group_enum_string = TYPEDEF_ENUM;
                    for (Group group : groups) {
                        /* add each group enum */
                        group_enum_string += getEnumString(group.getSanitizedName()) + ",\n";
                    }
                    group_enum_string += endEnumString(null);
                    write(group_enum_string);
                }
            }
        }
    }
}

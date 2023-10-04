package com.digi.connector.config;

import java.io.IOException;
import java.util.Map;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;

import com.digi.connector.config.ConfigGenerator.ItemType;
import com.digi.connector.config.ItemList.Capacity;

public class GenFsmSourceFile extends GenSourceFile {

    private final static String FILENAME = "remote_config.c";

    public GenFsmSourceFile() throws IOException {
        super(FILENAME, GenFile.Type.INTERNAL, GenFile.UsePrefix.NONE);
    }

    public void writeContent() throws Exception {
        writeFunctionFile();
    }

    private int prevRemoteStringLength;
    private String getDefineStringIndex(String define_name, String string, ConfigGenerator.FileType fileType) {
        String str = DEFINE + define_name + " " + "(" + CONNECTOR_REMOTE_ALL_STRING + "+" + prevRemoteStringLength + ")\n";
        /* NOTE: this prevRemoteStringLength is important.
         * Must keep track this variable for #define string.
         */
        if (string != null) {
            prevRemoteStringLength += string.length();
        }
        prevRemoteStringLength++; /* add extra byte for the size */

        if (fileType == options.fileTypeOption() ||
            options.fileTypeOption() == ConfigGenerator.FileType.NONE) {
            return str;
        } else {
            return "";
        }
    }

    private void writeDefineErrors(String prefixName, Map<String, String> errorMap, ConfigGenerator.FileType fileType) throws IOException {
        for (String key : errorMap.keySet()) {
            String defineName = prefixName.toUpperCase() + "_" + key.toUpperCase();
            /* define name string index for each error */
            write(getDefineStringIndex(defineName, errorMap.get(key), fileType));
        }
    }

    private void writeDefineRciErrors() throws IOException {
        if (!options.excludeErrorDescription()) {
            writeDefineErrors(GLOBAL_FATAL_PROTOCOL_ERROR, config.getGlobalFatalProtocolErrors(), ConfigGenerator.FileType.SOURCE);
            writeDefineErrors(GLOBAL_PROTOCOL_ERROR, config.getGlobalProtocolErrors(), ConfigGenerator.FileType.SOURCE);
        }
    }

    private String getDefineString(String define_name) {
        return (configType.toUpperCase() + "_" + define_name.toUpperCase());
    }

    private void writeDefineGroupErrors() throws Exception {
        for (Group.Type type : Group.Type.values()) {
            String defineName = null;
            Collection<Group> groups = config.getTable(type).groups();

            configType = type.toLowerName();

            for (Group group : groups) {
                defineName = getDefineString(group.getSanitizedName());
                /* define name string index
                 * #define [group name]
                 */

                if ((!options.excludeErrorDescription()) && (!group.getErrors().isEmpty())) {
                    LinkedHashMap<String, String> errorMap = group.getErrors();
                    for (String key : errorMap.keySet()) {
                        defineName = getDefineString(group.getSanitizedName() + "_" + ERROR + "_" + key);
                        /* define name string index for each error in the group
                         * #define [group name + ERROR + error name]
                         */
                        write(getDefineStringIndex(defineName, errorMap.get(key), ConfigGenerator.FileType.SOURCE));
                    }
                }
            }
        }

    }

    private void writeDefineGlobalErrors() throws IOException {
        if (!options.excludeErrorDescription()) {
            writeDefineErrors(GLOBAL_ERROR, config.getGlobalUserErrors(), ConfigGenerator.FileType.SOURCE);
        }
    }

    private Boolean isFirstRemoteString = true;
    private String getCharString(String string) {

        String quote_char = (isFirstRemoteString) ? "": ",\n";

        isFirstRemoteString = false;

        if (string != null) {

            char[] characters = string.toCharArray();

            quote_char += " " + string.length() + ",";

            int length = characters.length;

            for (int i=0; i < length; i++)
            {
                quote_char += "\'" + characters[i] + "\'";
                if (i < length-1) {
                    //last character
                    quote_char += ",";
                }
            }
        }
        else {
            quote_char += " 0";
        }

        return quote_char;
    }

    private void writeLinkedHashMapStrings(Map<String, String> stringMap) throws IOException {
        for (String key : stringMap.keySet()) {
            write(getCharString(stringMap.get(key)));
        }
    }

    private void writeRciErrorsRemoteAllStrings() throws IOException {
        writeLinkedHashMapStrings(config.getGlobalFatalProtocolErrors());
        writeLinkedHashMapStrings(config.getGlobalProtocolErrors());
    }

    private void writeGroupRemoteAllStrings(Collection<Group> groups) throws Exception {
        for (Group group : groups) {
            writeLinkedHashMapStrings(group.getErrors());
        }
    }

    protected void writeRemoteAllStrings() throws Exception {
        if (options.excludeErrorDescription()) {
            return;
        }

        write(String.format("\nstatic char CONST %s[] = {\n", CONNECTOR_REMOTE_ALL_STRING));

        writeRciErrorsRemoteAllStrings();

        for (Group.Type type : Group.Type.values()) {
            Collection<Group> groups = config.getTable(type).groups();

            configType = type.toLowerName();
            writeGroupRemoteAllStrings(groups);
        }
        writeLinkedHashMapStrings(config.getGlobalUserErrors());

        write("\n};\n\n"); // end of CONNECTOR_REMOTE_ALL_STRING
    }

    private String getRemoteString(String define_name) {
        return (" " + define_name.toUpperCase());

    }

    private String COMMENTED(String comment) {
        return "/* " + comment + " */";
    }

    private int writeErrorStructures(int errorCount, String prefix, Map<String, String> errorMap) throws IOException {
        String defineName = prefix.toUpperCase();

        for (String key : errorMap.keySet()) {
            write(getRemoteString(defineName + "_" + key));
            errorCount--;
            if (errorCount > 0) {
                write(",");
            }
            write(COMMENTED(key) + "\n");
        }

        return errorCount;
    }

    private void writeGlobalErrorStructures() throws IOException {
        if (!options.excludeErrorDescription()) {
            int errorCount = config.getGlobalFatalProtocolErrors().size() + config.getGlobalProtocolErrors().size() + config.getGlobalUserErrors().size();

            if (errorCount > 0) {
                write(String.format("static char const * const %ss[] = {\n", GLOBAL_ERROR));

                errorCount = writeErrorStructures(errorCount, GLOBAL_FATAL_PROTOCOL_ERROR, config.getGlobalFatalProtocolErrors());
                errorCount = writeErrorStructures(errorCount, GLOBAL_PROTOCOL_ERROR, config.getGlobalProtocolErrors());
                errorCount = writeErrorStructures(errorCount, GLOBAL_ERROR, config.getGlobalUserErrors());

                write("};\n\n");
            }
        }
        else {
            write(String.format("%s%ss NULL\n", DEFINE, GLOBAL_ERROR));
        }
    }

    private String getElementDefine(String type_name, String element_name) {
        return (String.format("connector_element_%s_%s", type_name, element_name));
    }

    private String getCollectionType(ItemList list) {
        boolean isFixedCapacity = (list.getCapacity() == Capacity.FIXED);
        boolean isDictionary = list.isDictionary();

        String type = "connector_collection_type_";
        type += (isFixedCapacity) ? "fixed_": "variable_";
        type += (isDictionary) ? "dictionary": "array";

        return type;
    }

    private String getCapacityInitializer(ItemList list, String varname) {
        String result;
        boolean isDictionary = list.isDictionary();

        if (isDictionary) {
            LinkedHashSet<String> keys = list.getKeys();

            if (keys.isEmpty()) {
                if (options.c99()) {
                    result = ".capacity.dictionary = ";
                } else {
                    result = "";
                }
                  result += "{ 0, NULL }";
            } else {
                String keys_name = varname + "_keys";

                result = "{ " + keys.size() + ", " + keys_name + " }";
            }
        } else {
            result = "{ " + list.getInstances() + " " + COMMENTED("instances")+ " }";
        }

        return result;
    }

    private void writeCollectionArray(ItemList items, String prefix) throws Exception {
        // Traverse down the tree to define all the lists first as they need to be defined before the collections that include them are.
        for (Item item: items.getItems()) {
            assert (item instanceof Element) || (item instanceof ItemList);

            String itemVariable = getDefineString(customPrefix + prefix + "__" + item.getSanitizedName()).toLowerCase();
            if (item instanceof Element) {
                Element element = (Element) item;
                String optional = options.useNames().contains(ItemType.ELEMENTS)
                    ? String.format("    \"%s\",\n", element.getName())
                    : "";

                if (element.getDefault() != null) {
                    write("connector_element_value_t const " + itemVariable + "_default = { ");
                    if (options.c99()) {
                        switch (element.getType()) {
                            case UINT32:
                            case HEX32:
                            case X_HEX32:
                                write(".unsigned_integer_value = ");
                                break;
                            case INT32:
                                write(".signed_integer_value = ");
                                break;
                            case FLOAT:
                                write(".float_value = ");
                                break;
                            case ON_OFF:
                                write(".on_off_value = ");
                                break;
                            case BOOLEAN:
                                write(".boolean_value = ");
                                break;
                            case ENUM:
                                write(".enum_value = ");
                                break;
                            case IPV4:
                            case FQDNV4:
                            case FQDNV6:
                            case DATETIME:
                            case REF_ENUM:
                            case STRING:
                            case MULTILINE_STRING:
                            case PASSWORD:
                            case MAC_ADDR:
                                write(".string_value = ");
                                break;
                           }
                    }
                    write(element.getDefaultValue() + " };\n\n");
                }

                write("static connector_element_t CONST " + itemVariable + "_element = {\n");
                write(optional);

                if (element.getDefault() == null) {
                    write("    NULL,\n");
                } else {
                    write("    &" + itemVariable + "_default,\n");
                }
                write("    " + getElementDefine("access", element.getAccess().name().toLowerCase()) + ",\n");

                if (options.rciParserOption() || options.useNames().contains(ItemType.VALUES)) {
                    String enum_struct;

                    if (element.getType() == Element.Type.ENUM) {
                        String define_name = getDefineString(prefix + "__" + element.getName() + "_enum");
                        String variableName = customPrefix + define_name.toLowerCase();

                        enum_struct = "{ ARRAY_SIZE(" + variableName + "), " + variableName + "}, ";
                    }
                    else {
                        enum_struct = "{ 0, NULL }, ";
                    }

                    write("    " + enum_struct + "\n");
                }
                write("};\n");
                write("\n");
            } else {
                ItemList subitems = (ItemList) item;
                String subitemsPrefix = prefix + "__" + item.getSanitizedName().toLowerCase();

                writeCollectionArray(subitems, subitemsPrefix);
                writeCollectionKeysArray(subitems.getKeys(), itemVariable);

                String subitemsVariable = itemVariable + "_items";
                String optional = options.useNames().contains(ItemType.COLLECTIONS)
                    ? String.format("    \"%s\",\n", subitems.getName())
                    : "";

                String type = getCollectionType(subitems);
                String capacity = getCapacityInitializer(subitems, itemVariable);

                write("static connector_collection_t CONST " + itemVariable + "_collection = {\n");
                write(optional);
                write("    " + type + ",\n");
                write("    " + capacity + ",\n");
                write("    {\n");
                write("        " + subitems.getItems().size() + ", " + COMMENTED("items") + "\n");
                write("        " + subitemsVariable + "\n");
                write("    }\n");
                write("};\n\n");
            }
        }

        // Then write the array for this level
        String itemsVariable = getDefineString(customPrefix + prefix).toLowerCase();
        boolean first = true;
        write("static connector_item_t CONST " + itemsVariable + "_items[] = {\n");
        for (Item item: items.getItems()) {
            assert (item instanceof Element) || (item instanceof ItemList);

            String type;
            String suffix;
            if (item instanceof Element) {
                Element element = (Element) item;

                type = element.getType().toLowerName();
                suffix = "element";
            } else {
                type = "list";
                suffix = "collection";
            }
            if (!first) {
                write(",\n");
            } else {
                first = false;
            }

            String itemVariable = getDefineString(customPrefix + prefix + "__" + item.getSanitizedName()).toLowerCase() + "_" + suffix;
            write("{ " + getElementDefine("type", type) + ", { ");
            if (options.c99()) {
                write("." + suffix + " = ");
            }
            write("&" + itemVariable + " } }");
        }
        write("\n};\n\n");
    }

    private void writeEnumArray(Element element, String prefix) throws Exception {
        boolean first = true;
        String define_name = getDefineString(prefix + "__" + element.getName() + "_enum");

        write(String.format("static connector_element_enum_t CONST %s%s[] = {\n",
                customPrefix, define_name.toLowerCase()));

        for (Value value : element.getValues()) {
            if (!first) {
                write(",\n");
            } else {
                first = false;
            }

            write(String.format("    {\"%s\"}",value.getName()));
        }
        write("\n};\n\n");
    }

    private void writeEnumArrays(ItemList items, String prefix) throws Exception{
        for (Item item: items.getItems()) {
            assert (item instanceof Element) || (item instanceof ItemList);

            if (item instanceof Element) {
                Element element = (Element) item;

                if (element.getType() == Element.Type.ENUM) {
                    writeEnumArray(element, prefix);
                }
            } else {
                ItemList subitems = (ItemList) item;

                writeEnumArrays(subitems, prefix + "__" + subitems.getSanitizedName());
            }
        }
    }

    private void writeCollectionArrays(ItemList items, String prefix) throws Exception {
        if (options.rciParserOption() || options.useNames().contains(ItemType.VALUES)) {
            writeEnumArrays(items, prefix);
        }

        writeCollectionArray(items, prefix);
    }

    private void writeLocalErrorStructures(String error_name, LinkedHashMap<String, String> localErrors) throws IOException {
        if (!options.excludeErrorDescription()) {
            int errorCount = localErrors.size();

            if (errorCount > 0) {
                String define_name = getDefineString(error_name + "_" + ERROR);

                write(CHAR_CONST_STRING + customPrefix + define_name.toLowerCase() + "s[] = {\n");
                writeErrorStructures(errorCount, define_name, localErrors);
                write("};\n\n");
            }
        }
    }

    private void writeGroupStructures(Collection<Group> groups) throws Exception {
        for (Group group: groups) {
            writeCollectionArrays(group, group.getSanitizedName());

            if (!options.excludeErrorDescription()) {
                writeLocalErrorStructures(group.getSanitizedName(), group.getErrors());
            }
        }
    }

    private void writeCollectionKeysArray(LinkedHashSet<String> keys, String varname) throws IOException {
        if (!keys.isEmpty()) {
            LinkedList<String> values = new LinkedList<>();
            for (String key: new LinkedList<String>(keys)) {
                values.add(Code.quoted(key));
            }

            write("static char const * const " + varname + "_keys[] = {\n");
            for (String line: Code.commas(values)) {
                write(Code.indented(line));
            }
            write("}\n");
            write("\n");
        }
    }

    private void writeAllStructures() throws Exception {
        for (Group.Type type: Group.Type.values()) {
            Collection<Group> groups = config.getTable(type).groups();

            configType = type.toLowerName();

            if (!groups.isEmpty()) {
                writeGroupStructures(groups);

                for (Group group: groups) {
                    if (group.isDictionary() && !group.getKeys().isEmpty()) {
                        writeCollectionKeysArray(group.getKeys(), customPrefix + getDefineString(group.getSanitizedName()));
                    }
                }

                write(String.format("static connector_group_t CONST %sconnector_%s_groups[] = {", customPrefix, configType));

                int remaining = groups.size();
                for (Group group: groups) {
                    String items_name = customPrefix + getDefineString(group.getSanitizedName() + "_items").toLowerCase();
                    String optional = options.useNames().contains(ItemType.COLLECTIONS)
                            ? String.format("        \"%s\",\n", group.getSanitizedName())
                            : "";

                    String ctype = getCollectionType(group);
                    String capacity = getCapacityInitializer(group, customPrefix + getDefineString(group.getSanitizedName()));
                    String group_string =
                        "\n" +
                        "{\n" +
                        "    {\n" +
                        optional +
                        "        " + ctype + ",\n" +
                        "        " + capacity + ",\n" +
                        "        { " + group.getItems().size() + ", " + items_name + " }, \n" +
                        "    },\n";

                    if ((!options.excludeErrorDescription()) && (!group.getErrors().isEmpty())) {
                        String errors_name = customPrefix + getDefineString(group.getSanitizedName() + "_errors").toLowerCase();

                        group_string += "    { ARRAY_SIZE(" + errors_name + "), " + errors_name + " }, \n";

                    } else {
                        group_string += "    { 0, NULL }\n";
                    }

                    remaining -= 1;
                    if (remaining != 0) {
                        group_string += "},\n";
                    } else {
                        group_string += "}\n";
                    }
                    write(group_string);
                }
                write("\n};\n\n");
            }
        }

        LinkedList<String> group_lines = new LinkedList<>();
        for (Group.Type type : Group.Type.values()) {
            Collection<Group> groups = config.getTable(type).groups();

            if (groups.isEmpty()) {
                group_lines.add("    { NULL, 0 }");
            } else {
                String var_name = customPrefix + "connector_" + type.toLowerName() + "_groups";

                group_lines.add(String.format("    { %s, ARRAY_SIZE(%s) }", var_name, var_name));
            }

        }

        write(
                String.format("static connector_remote_group_table_t CONST %s[] =\n", CONNECTOR_REMOTE_GROUP_TABLE) +
                "{\n" +
                String.join(",\n", group_lines) + "\n" +
                "};\n" +
                "\n");
    }

    private void writeFunctionFile() throws Exception
    {
        write(String.format("%s", CONNECTOR_GLOBAL_HEADER));

        /*
         * Start writing:
         * 1. all #define for all strings from user's groups
         * 2. all #define for all RCI and user's global errors
         * 3. all strings in connector_remote_all_strings[]
         */
        write("\n\n#define CONST const \n");

        /* Write Define Errors Macros */
        writeDefineRciErrors();
        writeDefineGroupErrors();
        writeDefineGlobalErrors();

        /* write remote all strings in source file */
        writeRemoteAllStrings();

        /* write connector_rci_errors[] */
        writeGlobalErrorStructures();

        /* write structures in source file */
        writeAllStructures();

        final int GlobalErrorCount = config.getGlobalFatalProtocolErrors().size() + config.getGlobalProtocolErrors().size() + config.getGlobalUserErrors().size();
        write(String.format("\nconnector_remote_config_data_t const %srci_internal_data = {\n" +
            "    connector_group_table,\n"+
            "    connector_global_errors,\n"+
            "    %d,\n"+
            "    0x%X,\n"+
            "    0x%08X,\n"+
            "    \"%s\"\n"+
            "};\n"+
            "\n"+
            "connector_remote_config_data_t const * const rci_descriptor_data = &%srci_internal_data;\n"
            , customPrefix, GlobalErrorCount, options.getFirmware(), options.getVendorId(), options.getDeviceType(), customPrefix));
    }
}

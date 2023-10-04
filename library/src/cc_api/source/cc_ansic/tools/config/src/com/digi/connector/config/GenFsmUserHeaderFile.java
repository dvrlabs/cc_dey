package com.digi.connector.config;

import java.io.IOException;
import java.util.Map;
import java.util.Collection;
import java.util.LinkedHashMap;

public class GenFsmUserHeaderFile extends GenHeaderFile {
    public final static String FILENAME = "rci_config.h";

    public GenFsmUserHeaderFile() throws IOException {
        super(FILENAME, GenFile.Type.USER, GenFile.UsePrefix.CUSTOM);
    }

    public void writeGuardedContent() throws Exception {
        writeGlobalErrorEnumHeader();
        writeGroupTypeAndErrorEnum();
    }

    private void writeErrorHeader(int errorIndex, String enumDefine, Map<String, String> errorMap) throws IOException {

        for (String key : errorMap.keySet()) {
            String error_string = " " + enumDefine + "_" + key;

            if (errorIndex == 1) {
                error_string += " = " + " " + enumDefine + "_" + OFFSET_STRING;
            }
            errorIndex++;

            error_string += ",\n";

            write(error_string);
        }

    }

    private void writeGlobalErrorEnumHeader() throws IOException {

        String index_string = "";
        /* write typedef enum for user global error */
        String enumName = GLOBAL_ERROR + "_" + OFFSET_STRING;

        write("\n" + TYPEDEF_ENUM + " " + enumName + " = " + GLOBAL_ERROR + "_" + COUNT_STRING + ",\n");

        writeErrorHeader(1, GLOBAL_ERROR, config.getGlobalUserErrors());

        String endString = String.format(" %s_%s%s", GLOBAL_ERROR, COUNT_STRING, index_string);

        if (config.getGlobalUserErrors().isEmpty()) {
            endString += " = " + enumName;
        }
        endString += "\n} " + customPrefix + GLOBAL_ERROR + ID_T_STRING;

        write(endString);

    }

    private String getEnumString(String enum_name) {
        String str = " " + customPrefix + CONNECTOR_PREFIX + "_" + configType;

        if (enum_name != null) {
            str += "_" + enum_name;
        }
        return str;
    }

    private String sanitizeName(String name) {
        return name.replace('-', '_').replace(".","_fullstop_");
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
        str +=ID_T_STRING;

        return str;
    }

    private void writeEnumHeader(Element element, String prefix) throws Exception {
        boolean explicit = false;
        int index = 0;

        write(TYPEDEF_ENUM);
        for (Value value : element.getValues()) {
            if (value.getName().equals(""))
                explicit = true;
            else {
                String line = getEnumString(prefix + "_" + sanitizeName(value.getName()));

                if (explicit) {
                    line += " = " + index;
                    explicit = false;
                }
                write(line + ",\n");
            }
            index++;
        }
        write(endEnumString(prefix));
    }

    private void writeListEnumHeader(ItemList list, String prefix) throws Exception {
        String element_enum_string = TYPEDEF_ENUM;

        for (Item item : list.getItems()) {
            String item_prefix = prefix + "_" + sanitizeName(item.getName());

            assert (item instanceof Element) || (item instanceof ItemList);

            if (item instanceof Element) {
                Element element = (Element) item;

                element_enum_string += getEnumString(item_prefix) + ",\n";

                if (element.getType() == Element.Type.ENUM) {
                    writeEnumHeader(element, item_prefix);
                }
            } else {
                ItemList sublist = (ItemList) item;

                writeListEnumHeader(sublist, item_prefix + "_");
            }
        }
        element_enum_string += endEnumString(prefix);

        write(element_enum_string);
    }

    private String COMMENTED(String comment) {
        return "/* " + comment + " */";
    }

    private void writeErrorHeader(String type, int errorIndex, String enumDefine, Map<String, String> errorMap) throws IOException {
        for (String key : errorMap.keySet()) {
            String error_string = enumDefine + "_" + key;

            if (type.equalsIgnoreCase("rci")){
                if (errorIndex == 1) error_string += " = 1, " + COMMENTED("Protocol defined") + "\n";
                else error_string += ",\n";
                errorIndex++;
            } else if (type.equalsIgnoreCase("global")){
                if (errorIndex == 1) error_string += ", " +  COMMENTED("User defined (global errors)") + "\n";
                else error_string += ",\n";
                errorIndex++;
            } else {
                error_string += ",\n";
            }
            write(error_string);
        }
    }

    private void writeAllEnumHeaders(Collection<Group> groups) throws Exception {

        for (Group group : groups) {
            writeListEnumHeader(group, group.getName());

            if (!group.getErrors().isEmpty()) {
                write(TYPEDEF_ENUM);

                writeErrorHeader("rci", 1, getEnumString(group.getName() + "_" + "ERROR"), config.getGlobalFatalProtocolErrors());
                writeErrorHeader("rci", 1, getEnumString(group.getName() + "_" + "ERROR"), config.getGlobalProtocolErrors());
                writeErrorHeader("global", 1, getEnumString(group.getName() + "_" + "ERROR"), config.getGlobalUserErrors());

                LinkedHashMap<String, String> errorMap = group.getErrors();
                int index = 0;

                for (String key : errorMap.keySet()) {
                    String enumString = getEnumString(group.getName() + "_" + "ERROR" + "_" + key);

                    if (index++ == 0) {
                        /*Set start index to the global count */
                        enumString += ", " + COMMENTED("User defined (group errors)") + "\n";
                    }
                    else{
                    enumString += ",\n";
                    }

                    write(enumString);
                }
                write(endEnumString(group.getName() + "_" + "ERROR"));
            }
        }
    }

    private void writeGroupTypeAndErrorEnum() throws Exception {

        for (Group.Type type : Group.Type.values()) {
            Collection<Group> groups = config.getTable(type).groups();

            configType = type.toLowerName();

            if (!groups.isEmpty()) {
                /* build group enum string for group enum */
                String group_enum_string = TYPEDEF_ENUM;

                /* Write all enum in H file */
                writeAllEnumHeaders(groups);

                for (Group group : groups) {
                    /* add each group enum */
                    group_enum_string += getEnumString(group.getName()) + ",\n";
                }

                /* write group enum buffer to fileWriter */
                group_enum_string += endEnumString(null);
                write(group_enum_string);
            }
        }
    }
}

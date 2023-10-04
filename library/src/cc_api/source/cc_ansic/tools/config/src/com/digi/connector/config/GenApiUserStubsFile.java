package com.digi.connector.config;

import java.io.IOException;
import java.util.Collection;

import com.digi.connector.config.ConfigGenerator.ItemType;

public class GenApiUserStubsFile extends GenSourceFile {

    private static String FILENAME = "ccapi_rci_functions.c";

    private GenApiUserHeaderFile header;
    public GenApiUserStubsFile(GenApiUserHeaderFile header) throws IOException {
        super(FILENAME, GenFile.Type.USER, GenFile.UsePrefix.CUSTOM);

        this.header = header;
    }

    public void writeContent() throws Exception {
        write(String.format("%s <%s>\n", INCLUDE, "stdio.h"));
        write(String.format("%s \"%s\"\n", INCLUDE, "ccapi/ccapi.h"));
        write(String.format("%s \"%s\"\n\n", INCLUDE, header.path.getFileName().toString()));

        writeFunctionsCB();
    }

    private String UNUSED(String parameter) {
        return "UNUSED_PARAMETER(" + parameter + ");\n";
    }

    private String setFunction (String groupName, String parameter, String value){

        String name = (groupName.equals("_SESSION_GROUP_"))
            ? "global"
            : configType + "_" + groupName;
        String retvalErrorType = "\n" + customPrefix + CCAPI_PREFIX + "_" + name + "_error_id_t ";
        String PRINTF = "    printf(\"    Called '%s'\\n\", __FUNCTION__);\n";
        String RETURN_CONTINUE = String.format("    return %sCCAPI_GLOBAL_ERROR_NONE;\n}\n", customPrefix.toUpperCase());
        String function = retvalErrorType + parameter + "\n{\n    " + UNUSED("info") + PRINTF;

        if (value != null) {
            function += "    " + value + ";\n";
        }
        function += RETURN_CONTINUE;

        return function;
    }

    private void writeItemFunctionsCB(String prefix, ItemList list) throws Exception {
        for (Item item : list.getItems()) {
            assert (item instanceof Element) || (item instanceof ItemList);

            if (item instanceof Element) {
                Element element = (Element) item;

                String element_function ="";
                String FType = "";
                String value = "";

                switch (element.getType()) {
                    case UINT32:
                        if (element.getMin()!=null)
                           value += "*value = " + element.getMin();
                        else
                            value += "*value = 123";
                        FType += "uint32_t";
                        break;
                    case HEX32:
                    case X_HEX32:
                        value += "*value = 0x20101010";
                        FType += "uint32_t";
                        break;
                    case INT32:
                        if (element.getMin()!=null)
                            value += "*value = " + element.getMin();
                        else
                            value += "*value = 15";
                        FType += "int32_t";
                        break;
                    case FLOAT:
                        value += "*value = 1.2";
                        FType += "float";
                        break;
                    case ON_OFF:
                        value += "*value = CCAPI_ON";
                        FType += "ccapi_on_off_t";
                        break;
                    case ENUM:
                        Value first_value = element.getValues().get(0);
                        if (options.rciParserOption() || options.useNames().contains(ItemType.VALUES)) {
                            value += "*value = \"" + first_value.getName() + "\"";
                            FType += "char const *";
                        } else {
                            value += "*value = " + customPrefix.toUpperCase() + "CCAPI_" + configType.toUpperCase() + "_" + prefix.toUpperCase() + "_" + element.getName().toUpperCase() + "_" + first_value.getName().replace(" ", "_").toUpperCase();
                            FType += String.format("%s%s_%s_%s_%s_id_t", customPrefix, CCAPI_PREFIX, configType, prefix, element.getName());
                        }

                        break;
                    case IPV4:
                    case FQDNV4:
                    case FQDNV6:
                        value += "*value = \"192.168.1.1\"";
                        FType += "char const *";
                        break;
                    case DATETIME:
                        value += "*value = \"2002-05-30T09:30:10-0600\";";
                        FType += "char const *";
                        break;
                    case STRING:
                    case MULTILINE_STRING:
                        value += "*value = \"String\"";
                        FType += "char const *";
                        break;
                    case PASSWORD:
                        FType += "char const *";
                        break;
                    case MAC_ADDR:
                        value += "*value = \"00:04:9D:AB:CD:EF\"";
                        FType += "char const *";
                        break;
                    case BOOLEAN:
                        value += "*value = CCAPI_TRUE";
                        FType += "ccapi_bool_t";
                        break;
                default:
                    break;
                }

                if (element.getType() != Element.Type.PASSWORD) {
                    element_function += setFunction(prefix, String.format("%srci_%s_%s_%s_get(%s, %s * const value)",
                        customPrefix, configType, prefix, element.getName(), RCI_INFO_T, FType), value);
                }

                if (element.getAccess() != Item.AccessType.READ_ONLY) {
                    String value_type_modifier = "";

                    switch (element.getType()) {
                        case ENUM:
                            if (options.rciParserOption() || options.useNames().contains(ItemType.VALUES)) {
                                break;
                            }
                            /* Intentional fall-thru */
                        case UINT32:
                        case HEX32:
                        case X_HEX32:
                        case INT32:
                        case FLOAT:
                        case ON_OFF:
                        case BOOLEAN:
                            value_type_modifier = "const *";
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
                        case LIST:
                            break;
                    }
                    element_function += setFunction(prefix, String.format("%srci_%s_%s_%s_set(%s, %s " + value_type_modifier + "const value)",
                        customPrefix, configType, prefix, element.getName(), RCI_INFO_T, FType), "UNUSED_PARAMETER(value)");
                }
                write(element_function);
            } else {
                ItemList items = (ItemList) item;

                writeItemFunctionsCB(prefix + "_" + items.getName(), items);
            }
        }
    }

    private void writeFunctionsCB() throws Exception {
        String session_function =
            setFunction("_SESSION_GROUP_", customPrefix + "rci_session_start_cb(" + RCI_INFO_T + ")", null) +
            setFunction("_SESSION_GROUP_", customPrefix + "rci_session_end_cb(" + RCI_INFO_T + ")", null) +
            setFunction("_SESSION_GROUP_", customPrefix + "rci_action_start_cb(" + RCI_INFO_T + ")", null) +
            setFunction("_SESSION_GROUP_", customPrefix + "rci_action_end_cb(" + RCI_INFO_T + ")", null);

        if (options.rciLegacyEnabled()) {
            session_function +=
                setFunction("_SESSION_GROUP_", "rci_do_command_cb(" + RCI_INFO_T + ")", null) +
                setFunction("_SESSION_GROUP_", "rci_set_factory_defaults_cb(" + RCI_INFO_T + ")", null) +
                setFunction("_SESSION_GROUP_", "rci_reboot_cb(" + RCI_INFO_T + ")", null);
        }
        write(session_function);

        for (Group.Type type : Group.Type.values()) {
            Collection<Group> groups = config.getTable(type).groups();

            configType = type.toLowerName();

            for (Group group : groups) {
                String group_function = setFunction(group.getName(), String.format("%srci_%s_%s_start(%s)",customPrefix,configType,group.getName(),RCI_INFO_T),null);
                group_function += setFunction(group.getName(), String.format("%srci_%s_%s_end(%s)",customPrefix,configType,group.getName(),RCI_INFO_T),null);
                write(group_function);

                writeItemFunctionsCB(group.getName(), group);
            }
        }
    }
}

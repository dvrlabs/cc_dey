package com.digi.connector.config;

import java.io.IOException;
import java.util.Collection;

public class GenApiUserSourceFile extends GenSourceFile {

    private static String FILENAME = "ccapi_rci_data.c";

    private GenApiUserHeaderFile header;
    public GenApiUserSourceFile(GenApiUserHeaderFile header) throws IOException {
        super(FILENAME, GenFile.Type.USER, GenFile.UsePrefix.CUSTOM);

        this.header = header;
    }

    public void writeContent() throws Exception {
        write(String.format("%s \"%s\"\n", INCLUDE, "ccapi/ccapi.h"));
        write(String.format("%s \"%s\"\n\n", INCLUDE, header.path.getFileName().toString()));

        /* write structures in source file */
        writeAllCcapiStructures();
    }

    private String getDefineString(String define_name) {
        return (configType.toUpperCase() + "_" + define_name.toUpperCase());
    }

    private String COMMENTED(String comment) {
        return "/* " + comment + " */";
    }

    private void writeItemArrays(String prefix, ItemList list) throws Exception {
        String current = String.format("static ccapi_rci_element_t const %s%s_elements[] = {",
                customPrefix, getDefineString(prefix).toLowerCase());

        for (Item item: list.getItems()) {
            assert (item instanceof Element) || (item instanceof ItemList);

            if (item instanceof Element) {
                Element element = (Element) item;

                current += String.format("\n { %s", COMMENTED(element.getName()));

                if (element.getAccess() == Item.AccessType.READ_ONLY) {
                    current += "   NULL,\n";
                }
                else {
                    current += String.format("   %s%srci_%s_%s_set,\n", RCI_FUNCTION_T, customPrefix, getDefineString(prefix).toLowerCase(),element.getName());
                }

                current += String.format("   %s%srci_%s_%s_get\n", RCI_FUNCTION_T, customPrefix, getDefineString(prefix).toLowerCase(),element.getName());
                current += " },";
            } else {
                ItemList items = (ItemList) item;

                writeItemArrays(prefix + "_" + items.getName(), items);

                // TODO: Not sure what to do about lists in the CCAPI version quite yet. Making them NULL for now to trigger an error. -ASK
                current +=
                    String.format("\n { %s", COMMENTED(items.getName())) +
                    "   NULL,\n" +
                    "   NULL\n" +
                    " },";
            }
        }

        if (current.endsWith(",")) {
            current = current.substring(0, current.length() - 1);
        }

        write(current + "\n};\n\n");
    }

    private void writeGroupStructures(Collection<Group> groups) throws Exception {
        for (Group group: groups) {
            /* write element structure */
            writeItemArrays(group.getName(), group);
        }
    }

    private void writeAllCcapiStructures() throws Exception {
        String define_name;

        for (Group.Type type : Group.Type.values()) {
            Collection<Group> groups = config.getTable(type).groups();

            configType = type.toString().toLowerCase();

            if (!groups.isEmpty()) {
                writeGroupStructures(groups);

                write(String.format("static ccapi_rci_group_t const %sccapi_%s_groups[] =\n{", customPrefix, configType));

                int remaining = groups.size();
                for (Group group: groups) {
                    define_name = getDefineString(group.getName() + "_elements");

                    String group_string = "";
                    group_string += String.format("\n    { %s", COMMENTED(group.getName()));
                    group_string += String.format("        %s%s,", customPrefix, define_name.toLowerCase());
                    group_string += String.format("\n        ARRAY_SIZE(%s%s),", customPrefix, define_name.toLowerCase());
                    group_string += String.format("\n        {");
                    group_string += String.format("\n            %s%srci_%s_start," , RCI_FUNCTION_T,customPrefix,getDefineString(group.getName()).toLowerCase());
                    group_string += String.format("\n            %s%srci_%s_end" , RCI_FUNCTION_T,customPrefix,getDefineString(group.getName()).toLowerCase());
                    group_string += String.format("\n        }");
                    group_string += String.format("\n    }");

                    remaining -= 1;
                    if (remaining != 0) {
                        group_string += String.format(",");
                    }
                    write(group_string);
                }
                write("\n};\n\n");
            }
        }

        String ccfsm_internal_data = "extern connector_remote_config_data_t const " + customPrefix + "rci_internal_data;\n";
        String ccapi_rci_data = ccfsm_internal_data + "ccapi_rci_data_t const " + customPrefix + "ccapi_rci_data =\n{";

        for (Group.Type type: Group.Type.values()) {
            Collection<Group> groups = config.getTable(type).groups();

            ccapi_rci_data += String.format("\n    {");
            if (groups.isEmpty()) {
                ccapi_rci_data += String.format("\n        NULL,");
                ccapi_rci_data += String.format("\n        0");
            }
            else {
                configType = type.toLowerName();
                ccapi_rci_data += String.format("\n        %sccapi_%s_groups,", customPrefix, configType);
                ccapi_rci_data += String.format("\n        ARRAY_SIZE(%sccapi_%s_groups)", customPrefix, configType);
            }
            ccapi_rci_data += String.format("\n    },");
        }

        ccapi_rci_data += String.format("\n    {");
        ccapi_rci_data += String.format("\n        %s%srci_session_start_cb,", RCI_FUNCTION_T, customPrefix);
        ccapi_rci_data += String.format("\n        %s%srci_session_end_cb,", RCI_FUNCTION_T, customPrefix);
        ccapi_rci_data += String.format("\n        %s%srci_action_start_cb,", RCI_FUNCTION_T, customPrefix);
        ccapi_rci_data += String.format("\n        %s%srci_action_end_cb", RCI_FUNCTION_T, customPrefix);
        ccapi_rci_data += String.format("\n        %s%srci_do_command_cb,", RCI_FUNCTION_T, customPrefix);
        ccapi_rci_data += String.format("\n        %s%srci_set_factory_defaults_cb,", RCI_FUNCTION_T, customPrefix);
        ccapi_rci_data += String.format("\n        %s%srci_reboot_cb", RCI_FUNCTION_T, customPrefix);
        ccapi_rci_data += String.format("\n    },");

        ccapi_rci_data += String.format("\n    &%srci_internal_data", customPrefix);
        ccapi_rci_data += "\n};\n\n";

        write(ccapi_rci_data);
    }
}

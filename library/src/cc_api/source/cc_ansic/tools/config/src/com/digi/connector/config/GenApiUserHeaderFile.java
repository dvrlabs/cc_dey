package com.digi.connector.config;

import java.io.IOException;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.Map.Entry;
import java.util.Set;

import com.digi.connector.config.ConfigGenerator.ItemType;

public final class GenApiUserHeaderFile extends GenHeaderFile {

    private static final String FILENAME = "ccapi_rci_functions.h";

    public GenApiUserHeaderFile() throws IOException {
        super(FILENAME, GenFile.Type.USER, GenFile.UsePrefix.CUSTOM);
    }

    private Map<String, Integer> getErrors(String prefix, Set<String> keys) {
        Map<String, Integer> result = new LinkedHashMap<>();

        result.put(prefix + "_NONE", result.size());
        for (String key: config.getGlobalFatalProtocolErrors().keySet()) {
            result.put(prefix + "_" + key.toUpperCase(), result.size());
        }
        for (String key: config.getGlobalProtocolErrors().keySet()) {
            result.put(prefix + "_" + key.toUpperCase(), result.size());
        }
        for (String key: config.getGlobalUserErrors().keySet()) {
            result.put(prefix + "_" + key.toUpperCase(), result.size());
        }
        for (String key: keys) {
            result.put(prefix + "_" + key.toUpperCase(), result.size());
        }
        result.put(prefix + "_COUNT", result.size());

        return result;
    }
    private Map<String, Integer> getErrors(String prefix) { return getErrors(prefix, Collections.<String> emptySet()); }

    private Map<String, Integer> getValues(String prefix, Element element) {
        Map<String, Integer> result = new LinkedHashMap<>();

        for (Value value: element.getValues()) {
            result.put(prefix + "_" + value.getSanitizedName().toUpperCase(), result.size());
        }
        result.put(prefix + "_COUNT", result.size());

        return result;
    }

    private void writeEnumValues(String prefix, ItemList list) throws Exception {
        for (Item item : list.getItems()) {
            String partial = prefix + "_" + item.getSanitizedName();

            if (item instanceof ItemList) {
                ItemList items = (ItemList) item;

                writeEnumValues(partial, items);
                continue;
            }

            assert (item instanceof Element);
            Element element = (Element) item;

            if (element.getType() != Element.Type.ENUM) {
                continue;
            }

            writeBlock(Code.enumerationTypedef(partial + "_id_t", getValues(prefix.toUpperCase(), element)));
        }
    }

    public final void writeGuardedContent() throws Exception {
        LinkedList<SimpleImmutableEntry<Group.Type, Group>> entries = config.getConfigGroupEntries();

        // Common
        {
            LinkedList<String> lines = new LinkedList<>();

            lines.add(Code.include("connector_api.h"));
            lines.add(Code.define("UNUSED_PARAMETER(a)", "(void)(a)"));
            lines.add("extern ccapi_rci_data_t const " + customPrefix + "ccapi_rci_data;");
            writeBlock(lines);
        }

        // Global errors
        {
            String prefix = customPrefix + String.join("_", CCAPI_PREFIX, "global", "error");
            writeBlock(Code.enumerationTypedef(prefix + "_id_t", getErrors(prefix.toUpperCase())));
        }

        // Group errors
        for (Entry<Group.Type, Group> entry : entries) {
            String type = entry.getKey().toLowerName();
            Group group = entry.getValue();

            String prefix = customPrefix + String.join("_", CCAPI_PREFIX, type, group.getSanitizedName());
            writeBlock(Code.enumerationTypedef(prefix + "_id_t", getErrors(prefix.toUpperCase(), group.getErrors().keySet())));
        }

        // All enumeration values
        for (Entry<Group.Type, Group> entry : entries) {
            String type = entry.getKey().toLowerName();
            Group group = entry.getValue();

            String prefix = customPrefix + String.join("_", CCAPI_PREFIX, type, group.getSanitizedName());
            writeEnumValues(prefix, group);
        }

        // Group prototypes
        for (Entry<Group.Type, Group> entry : entries) {
            String type = entry.getKey().toLowerName();
            Group group = entry.getValue();

            String group_prefix =  customPrefix + String.join("_", CCAPI_PREFIX, type, group.getSanitizedName());
            String group_return_type = group_prefix + "_error_id_t ";

            LinkedList<String> functions = new LinkedList<>();
            Collections.addAll(functions,
                group_prefix + "_start_cb",
                group_prefix + "_end_cb");
            writeBlock(Code.prototypes(group_return_type, functions, CCAPI_CB_PARAMETERS));
        }

        // Down-level prototypes
        for (Entry<Group.Type, Group> entry : entries) {
            String type = entry.getKey().toLowerName();
            Group group = entry.getValue();

            String group_prefix =  customPrefix + String.join("_", CCAPI_PREFIX, type, group.getSanitizedName());
            String group_return_type = group_prefix + "_error_id_t ";

            writeItemPrototypes(group_prefix, group, group_return_type);
        }

        // Top-level prototypes
        String return_type = customPrefix + CCAPI_PREFIX + "_global_error_id_t ";
        LinkedList<String> functions = new LinkedList<>();

        Collections.addAll(functions, "rci_session_start_cb", "rci_session_end_cb", "rci_action_start_cb", "rci_action_end_cb", "rci_do_command_cb", "rci_set_factory_defaults_cb", "rci_reboot_cb");
        writeBlock(Code.prototypes(return_type, functions, CCAPI_CB_PARAMETERS));
    }

    private void writeItemPrototypes(String prefix, ItemList list, String retval) throws Exception {
        for (Item item : list.getItems()) {
            if (item instanceof ItemList) {
                ItemList items = (ItemList) item;

                writeItemPrototypes(prefix + "_" + items.getSanitizedName(), items, retval);
                continue;
            }

            assert (item instanceof Element);
            Element element = (Element) item;
            String function = prefix + "_" + element.getSanitizedName();

            Code.Type value_type = null;
            switch (element.getType()) {
            case UINT32:
            case HEX32:
            case X_HEX32:
                value_type = UINT32;
                break;
            case INT32:
                value_type = UINT32;
                break;
            case FLOAT:
                value_type = UINT32;
                break;
            case ON_OFF:
                value_type = Code.Type.base("ccapi_on_off_t");
                break;
            case ENUM:
                if (options.rciParserOption() || options.useNames().contains(ItemType.VALUES)) {
                    value_type = CHAR.constant().pointer();
                } else {
                    value_type = Code.Type.base(prefix + "_" + element.getSanitizedName() + "_id_t");
                }
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
                value_type = CHAR.constant().pointer();
                break;
            case BOOLEAN:
                value_type = Code.Type.base("ccapi_bool_t");
                break;
            case LIST:
                break;
            }
            assert value_type != null;

            if (element.getAccess() == Item.AccessType.WRITE_ONLY) {
                writeLine(Code.define(function + "_get", "NULL"));
            } else {
                writeLine(Code.prototype(retval, function + "_get", Code.parameters(CCAPI_CB_PARAMETERS, value_type.constant().named("value"))));
            }

            if (element.getAccess() == Item.AccessType.READ_ONLY) {
                writeLine(Code.define(function + "_set", "NULL"));
            }
            else {
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
                        value_type = value_type.constant().pointer();
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
                writeLine(Code.prototype(retval, function + "_get", Code.parameters(CCAPI_CB_PARAMETERS, value_type.constant().named("value"))));
            }
        }
    }
}

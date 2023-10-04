package com.digi.connector.config;

import java.io.IOException;
import java.util.EnumMap;
import java.util.EnumSet;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.Arrays;
import java.util.Collection;

import com.digi.connector.config.ConfigGenerator.ItemType;
import com.digi.connector.config.Element;

public class Config {
    private EnumMap<Group.Type, Table> table = new EnumMap<>(Group.Type.class);
    private void initCollection(Group.Type type) {
        assert !table.containsKey(type) && table.size() < 2 : "invalid initialization of table";
        table.put(type, new Table(type));
    }

    private static Config instance = null;
    private Config() {
        initCollection(Group.Type.SETTING);
        initCollection(Group.Type.STATE);

        globalFatalProtocolErrorsOffset = 1;
        globalFatalProtocolErrors = new LinkedHashMap<>();
        globalFatalProtocolErrors.put("bad_command", "Bad command");
        globalFatalProtocolErrors.put("bad_descriptor", "Bad configuration");
        globalFatalProtocolErrors.put("bad_value", "Bad value");

        globalProtocolErrorsOffset = globalFatalProtocolErrorsOffset + globalFatalProtocolErrors.size();
        globalProtocolErrors = new LinkedHashMap<>();
        globalProtocolErrors.put("bad_value", "Bad value");
        globalProtocolErrors.put("invalid_index", "Invalid index");
        globalProtocolErrors.put("invalid_name", "Invalid name");
        globalProtocolErrors.put("missing_name", "Missing name");

        globalUserErrorsOffset = globalProtocolErrorsOffset + globalProtocolErrors.size();
        globalUserErrors = new LinkedHashMap<>();

        for (ItemType name: ItemType.values()) {
            max_name_length_seen.put(name, 0);
        }

        instance = this;
    }
    public static final Config getInstance() { if (instance == null) instance = new Config(); return instance; }

    private int globalFatalProtocolErrorsOffset;
    private final Map<String, String> globalFatalProtocolErrors;

    private int globalProtocolErrorsOffset;
    private Map<String, String> globalProtocolErrors;

    private int globalUserErrorsOffset;
    private Map<String, String> globalUserErrors;

    private int max_attribute_length = 20;
    private int max_list_depth = 0;
    private int max_dynamic_key_length = 0;
//    TODO: We don't use this today, but when we add static dictionaries we will need to add it. -ASK
//    private int max_static_key_length;
    private int max_name_length = 40;

    private LinkedList<Element> ref_enums = new LinkedList<>();

    private EnumMap<ItemType, Integer> max_name_length_seen = new EnumMap<>(ItemType.class);
    private EnumSet<Element.Type> typesSeen = EnumSet.noneOf(Element.Type.class);
    private boolean variableGroupSeen = false;
    private boolean variableListSeen = false;
    private boolean dictionarySeen = false;
    private boolean variableArraySeen = false;
    private boolean variableDictSeen = false;

    public Table getTable(Group.Type type) {
        return table.get(type);
    }

    public LinkedList<SimpleImmutableEntry<Group.Type, Group>> getConfigGroupEntries() {
        LinkedList<SimpleImmutableEntry<Group.Type, Group>> result = new LinkedList<>();

        for (Group.Type type: Group.Type.values()) {
            Collection<Group> groups = getTable(type).groups();

            for (Group group : groups) {
                result.add(new SimpleImmutableEntry<Group.Type, Group>(type, group));
            }
        }
        return result;
    }

    public boolean isProtocolGlobalError(String name) {
        return (globalFatalProtocolErrors.containsKey(name) || globalProtocolErrors.containsKey(name));
    }

    public boolean isUserGlobalError(String name) {
        return globalUserErrors.containsKey(name);
    }

    public void addUserGlobalError(String name, String description) throws Exception {

        if (description == null) {
            throw new IOException("Missing or bad globalerror description");
        }

        if (isProtocolGlobalError(name)) {
            throw new Exception("Existing protocol error <globalerror>: " + name);
        }

        if (isUserGlobalError(name)) {
            throw new Exception("Duplicate <globalerror>: " + name);
        }

        globalUserErrors.put(name, description);
    }

    public int getGlobalFatalProtocolErrorsOffset() {
        return globalFatalProtocolErrorsOffset;
    }

    public Map<String, String> getGlobalFatalProtocolErrors() {
        return globalFatalProtocolErrors;
    }

    public int getGlobalProtocolErrorsOffset() {
        return globalProtocolErrorsOffset;
    }

    public Map<String, String> getGlobalProtocolErrors() {
        return globalProtocolErrors;
    }

    public int getGlobalUserErrorsOffset() {
        return globalUserErrorsOffset;
    }

    public Map<String, String> getGlobalUserErrors() {
        return globalUserErrors;
    }

    public int getGroupErrorsOffset() {
        return globalUserErrorsOffset + globalUserErrors.size();
    }

    public void listDepth(int depth) {
        if (depth > max_list_depth) {
            max_list_depth = depth;
        }
    }

    public int getMaxDepth() {
        return max_list_depth;
    }

    public void addTypeSeen(Element.Type type) {
        typesSeen.add(type);
    }

    public EnumSet<Element.Type> getTypesSeen() {
        return typesSeen;
    }

    public void nameLengthSeen(ItemType type, int length) {
        int current = max_name_length_seen.get(type);
        max_name_length_seen.put(type, Math.max(current, length));
    }

    public int getMaxNameLengthSeen(ItemType type) {
        return max_name_length_seen.get(type);
    }

    public void setMaxAttributeLength(Integer length) throws Exception {
        if (length != null)
            max_attribute_length = length;
    }

    public int getMaxAttributeLength() {
        return max_attribute_length;
    }

    public void setMaxDynamicKeyLength(Integer length) {
        if (length != null)
            max_dynamic_key_length = length;
    }

    public int getMaxDynamicKeyLength() {
        return max_dynamic_key_length;
    }

    public void setMaxNameLength(Integer length) {
        if (length != null)
            max_name_length = length;
    }

    public int getMaxNameLength() {
        return max_name_length;
    }

    public void addRefEnum(Element ref_enum) {
        ref_enums.add(ref_enum);
    }

    public boolean isVariableGroupSeen() {
        return variableGroupSeen;
    }

    public void variableGroupSeen() {
        variableGroupSeen = true;
    }

    public boolean isVariableListSeen() {
        return variableListSeen;
    }

    public void variableListSeen() {
        variableListSeen = true;
    }
    
    public void dictionarySeen() {
        dictionarySeen = true;
    }

    public boolean isDictionarySeen() {
        return dictionarySeen;
    }

    public boolean isVariableArraySeen() {
        return variableArraySeen;
    }

    public void variableArraySeen() {
        variableArraySeen = true;
    }

    public boolean isVariableDictSeen() {
        return variableDictSeen;
    }

    public void variableDictSeen() {
        variableDictSeen = true;
    }

    private boolean match_path(ItemList list, String[] remaining) {
        switch (remaining.length) {
            case 0:
                return list.isDictionary();
            case 1:
                return false; // last part is an instance, which is in error
            default:
            {
                final String instance = remaining[0];
                if (list.isArray()) {
                    try {
                        final int index = Integer.parseInt(instance);
                        if (index > list.getInstances()) {
                            return false;
                        }

                    } catch (NumberFormatException e) {
                        return false;
                    }
                }

                final String next = remaining[1];
                for (Item item: list.getItems()) {
                    if (item.getName().equals(next) && (item instanceof ItemList)) {
                        return match_path((ItemList) item, Arrays.copyOfRange(remaining, 2, remaining.length));
                    }
                }

                return false;
            }
        }
    }

    public void validate() throws Exception {
        if (ref_enums.isEmpty()) {
            return;
        }

        Collection<Group> settings = getTable(Group.Type.SETTING).groups();
        for (Element ref_enum: ref_enums) {
            int min_length = Integer.MAX_VALUE;
            int max_length = 0;
            for (Value value: ref_enum.getValues()) {
                min_length = Math.min(min_length, value.name.length());
                max_length = Math.max(max_length, value.name.length());
            }
            for (Reference ref: ref_enum.getRefs()) {
                String[] parts = ref.getName().substring(1).split("/");
                String top = parts[0];
                String[] remaining = Arrays.copyOfRange(parts, 1, parts.length);

                boolean found = false;
                for (Group group: settings) {
                    if (group.getName().equals(top)) {
                        found = match_path(group, remaining);
                        break;
                    }
                }

                if (!found) {
                    throw new Exception("ref_enum has invalid reference: " + ref.getName());
                }

                min_length = Math.min(min_length, ref.getName().length() + max_dynamic_key_length);
                max_length = Math.max(max_length, ref.getName().length() + max_dynamic_key_length);
            }

            ref_enum.setMin(String.valueOf(min_length));
            ref_enum.setMax(String.valueOf(max_length));
        }

        for (Table t: table.values()) {
            t.validate();
        }
    }
}

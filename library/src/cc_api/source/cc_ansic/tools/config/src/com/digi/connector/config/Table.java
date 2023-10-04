package com.digi.connector.config;

import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Objects;
import java.util.Vector;

public class Table {
    private Group.Type type;
    private LinkedHashMap<String, Group> groups = new LinkedHashMap<>();
    private LinkedHashMap<String, Condition> conditions = new LinkedHashMap<>();

    public Table(Group.Type type) {
        this.type = type;
    }

    public int size() {
        return groups.size();
    }

    public Collection<Group> groups() {
        return groups.values();
    }

    public void addGroup(Group group) throws Exception {
        final String name = group.getName();

        if (groups.containsKey(name)) {
            throw new Exception("Duplicate <group> name: " + name);
        }

        groups.put(name, group);
    }

    public void addCondition(Condition condition) throws Exception {
        final String name = condition.getName();

        if (conditions.containsKey(name)) {
            throw new Exception("Duplicate <condition> name: " + name);
        }

        conditions.put(name, condition);
    }

    public Map<String, Condition> conditions() {
        return Collections.unmodifiableMap(conditions);
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

    public org.dom4j.Element generateQueryDescriptor(Config config, Integer id) throws Exception {
        final String desc = (type == Group.Type.SETTING)
                ? "Retrieve device configuration"
                : "Retrieve device state";
        final String config_type = type.toLowerName();
        final String formatName = String.format("all_%ss_groups", config_type);

        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("descriptor")
               .addAttribute("element", "query_" + config_type)
            .addAttribute("desc", desc)
            .addAttribute("format", formatName)
            .addAttribute("bin_id", id.toString());

        if (type == Group.Type.SETTING) {
            org.dom4j.Element attr;

            attr = e.addElement("attr")
                .addAttribute("name", "source")
                .addAttribute("type", "enum")
                .addAttribute("desc", "Source of settings returned")
                .addAttribute("bin_id", "0")
                .addAttribute("default", "current");

            attr.addElement("value")
                .addAttribute("value", "current")
                .addAttribute("desc", "Current settings")
                .addAttribute("bin_id", "0");

            attr.addElement("value")
                .addAttribute("value", "stored")
                .addAttribute("desc", "Settings stored in flash")
                .addAttribute("bin_id", "1");

            attr.addElement("value")
                .addAttribute("value", "defaults")
                .addAttribute("desc", "Device defaults")
                .addAttribute("bin_id", "2");

            attr = e.addElement("attr")
                .addAttribute("name", "compare_to")
                .addAttribute("type", "enum")
                .addAttribute("desc", "Return only differences from this source")
                .addAttribute("bin_id", "1")
                .addAttribute("default", "none");

            attr.addElement("value")
                .addAttribute("value", "none")
                .addAttribute("desc", "Return all settings")
                .addAttribute("bin_id", "0");

            attr.addElement("value")
                .addAttribute("value", "current")
                .addAttribute("desc", "Current settings")
                .addAttribute("bin_id", "1");

            attr.addElement("value")
                .addAttribute("value", "stored")
                .addAttribute("desc", "Settings stored in flash")
                .addAttribute("bin_id", "2");

            attr.addElement("value")
                .addAttribute("value", "defaults")
                .addAttribute("desc", "Device defaults")
                .addAttribute("bin_id", "3");
        }

        org.dom4j.Element format = e.addElement("format_define");
        format.addAttribute("name", formatName);

        // Add global errors at the format level
        addErrorDescriptors(format, config.getGlobalUserErrorsOffset(), config.getGlobalUserErrors());

        int gid = 0;
        for (Group group : groups()) {
            format.add(group.asElement(config, gid));
            gid++;
        }

        return e;
    }

    public org.dom4j.Element generateSetDescriptor(Config config, Integer id) throws Exception {
        final String desc = (type == Group.Type.SETTING)
                ? "Set device configuration"
                : "Set device state";
        final String config_type = type.toLowerName();
        final String formatName = String.format("all_%ss_groups", config_type);

        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("descriptor")
            .addAttribute("element", "set_" + config_type)
            .addAttribute("desc", desc)
            .addAttribute("format", formatName)
            .addAttribute("bin_id", id.toString());

        if (type == Group.Type.SETTING) {
            org.dom4j.Element attr;

            attr = e.addElement("attr")
                .addAttribute("name", "embed_transformed_values")
                .addAttribute("type", "boolean")
                .addAttribute("desc", "Specify whether transformed values should be embedded in the response")
                .addAttribute("bin_id", "0")
                .addAttribute("default", "false");
        }

        // FIX ME: REMOVE NEXT LINE
        addErrorDescriptors(e, config.getGlobalUserErrorsOffset(), config.getGlobalUserErrors());

        return e;
    }

    public void validate() throws Exception {
        for (Map.Entry<String, Condition> item: conditions.entrySet()) {
            final String name = item.getKey();
            final Location location = item.getValue().getLocation();

            assert type == location.getType() : "Something terrible happened";

            final Vector<String> path = location.getPath();
            final int length = path.size();

            try {
                Item current = groups.get(path.get(0));
                for (int i = 1; i < length; i++) {
                    if (current instanceof ItemList) {
                        ItemList list = (ItemList) current;
                        current = list.find(path.get(i));
                    } else {
                        throw new NoSuchElementException();
                    }
                }
                if (current instanceof Element) {
                    // TODO: check Element data for match validity -ASK
                    // enums & ref_enums must match one of the listed items
                    // boolean on/off must match true/false on/off
                    // numeric values must match numbers
                    // Possibilities for regex?
                } else {
                    throw new Exception("conditional source path does not end at an element: " + name);
                }
            } catch (NoSuchElementException e) {
                throw new Exception("conditional source path is invalid: " + name);
            }
        }

        // TODO: In the future we can warn when we have unused conditions. -ASK
    }
}

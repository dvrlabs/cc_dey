package com.digi.connector.config;

import java.io.IOException;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Objects;

public class Group extends ItemList {
    public enum Type {
        SETTING,
        STATE;

        public String toUpperName() {
            return name();
        }

        public String toLowerName() {
            return toUpperName().toLowerCase();
        }

        public static Type toType(String str) throws Exception {
            try {
                return valueOf(str.toUpperCase());
            } catch (Exception e) {
                throw new Exception("Invalid group Type: " + str);
            }
        }
    }

    private final LinkedHashMap<String, String> errorMap;

    public Group(String name, String description,
            String helpDescription) throws Exception {
        super(name, description, helpDescription);

        errorMap = new LinkedHashMap<String, String>();
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

    public org.dom4j.Element asElement(final Config config, final Integer id) {
        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("descriptor")
            .addAttribute("element", name)
            .addAttribute("desc", getRciDescription())
             .addAttribute("bin_id", id.toString());

         addAttributes(e);
        // FIXME: REMOVE NEXT LINE
         addErrorDescriptors(e, config.getGlobalUserErrorsOffset(), config.getGlobalUserErrors());
        addErrorDescriptors(e, config.getGroupErrorsOffset(), getErrors());
        addChildren(e);

         return wrapConditional(e);
    }

    public LinkedHashMap<String, String> getErrors() {
        return errorMap;
    }

    public void addError(String name, String description) throws IOException {
        if (errorMap.containsKey(name)) {
            throw new IOException("Duplicate <error>: " + name);
        }
        if (description == null) {
            throw new IOException("Missing or bad error description");
        }
        errorMap.put(name, description);
    }
}

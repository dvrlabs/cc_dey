package com.digi.connector.config;

import java.io.IOException;

public abstract class Item {

    private static final String[] invalid = { "error", "warning" };
    protected final String name;
    protected final String description;
    protected final String helpDescription;
    protected AccessType access;
    protected org.dom4j.Element wrapper;

    public enum AccessType {
        READ_ONLY, WRITE_ONLY, READ_WRITE;

        public static AccessType toAccessType(String str) throws Exception {
            try {
                return valueOf(str.toUpperCase());

            } catch (Exception e) {
                throw new Exception("Invalid access Type: " + str);
            }
        }

        public String toString() {
            return name().toLowerCase();
        }
    }

    public Item(String name, String description, String helpDescription) throws IOException {

        if (description == null) {
            throw new IOException("Missing or bad description");
        }

        for (String test: invalid) {
            if (name.equalsIgnoreCase(test)) {
                throw new IOException("Invalid name: " + name);
            }
        }
        this.name = name;
        this.description = description;
        this.helpDescription = helpDescription;
    }

    public String getRciDescription() {
        return (helpDescription == null)
            ? getDescription()
            : getDescription() + ":" + getHelpDescription();
    }

    public String getName() {
        return name;
    }

    public String getSanitizedName() {
        return name.replace('-', '_').replace(".","_fullstop_");
    }

    public String getDescription() {
        return description;
    }

    public String getHelpDescription() {
        return helpDescription;
    }

    public AccessType getAccess() {
        return access;
    }

    public void setAccess(String access) throws Exception {
        if (this.access == null)
            this.access = AccessType.toAccessType(access);
        else
            throw new Exception("Duplicate <access> keyword: " + access);
    }

    public void setCondition(final String name, final Location current, final Config config) throws Exception {
        assert wrapper == null : "Error attempting to overwrite condition";

        final Condition condition = config.getTable(current.getType()).conditions().get(name);
        if (condition == null) {
            throw new Exception("Condition not found: " + name);
        }

        wrapper = condition.wrapper(current);
    }

    public org.dom4j.Element wrapConditional(org.dom4j.Element element) {
        if (wrapper == null)
            return element;

        wrapper.add(element);
        return wrapper;
    }

    abstract org.dom4j.Element asElement(Integer id);
}

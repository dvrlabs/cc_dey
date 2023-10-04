package com.digi.connector.config;

import java.util.LinkedList;
import java.util.NoSuchElementException;
import java.util.LinkedHashSet;

import javax.naming.NamingException;

public class ItemList extends Item {
    public enum Capacity { FIXED, VARIABLE };

    private Integer instances = null;
    private LinkedHashSet<String> keys = null;
    private Capacity capacity = null;
    private final LinkedList<Item> items = new LinkedList<>();

    public ItemList(String name, String description, String helpDescription) throws Exception {
        super(name, description, helpDescription);
    }

    protected final org.dom4j.Element addAttributes(org.dom4j.Element e) {
        if (isDictionary()) {
            final int MAX_KEY_LENGTH = 64; // TODO: This should be configurable. -ASK
            e.addElement("attr")
                .addAttribute("bin_id", "0")
                .addAttribute("name", "name")
                .addAttribute("desc", "key name")
                .addAttribute("type", "string")
                .addAttribute("max", String.valueOf(MAX_KEY_LENGTH));

            if (capacity == ItemList.Capacity.VARIABLE) {
                e.addElement("attr")
                    .addAttribute("bin_id", "1")
                    .addAttribute("name", "complete")
                    .addAttribute("desc", "all keys given")
                    .addAttribute("type", "boolean")
                    .addAttribute("default", "false");
                e.addElement("attr")
                    .addAttribute("bin_id", "2")
                    .addAttribute("name", "remove")
                    .addAttribute("desc", "remove key")
                    .addAttribute("type", "boolean")
                    .addAttribute("default", "false");
            }
        } else {
            e.addElement("attr")
                .addAttribute("bin_id", "0")
                .addAttribute("name", "index")
                .addAttribute("desc", "item number")
                .addAttribute("type", "int32")
                .addAttribute("min", "1")
                .addAttribute("max", String.valueOf(instances));

            if (capacity == ItemList.Capacity.VARIABLE) {
                e.addElement("attr")
                    .addAttribute("bin_id", "1")
                    .addAttribute("name", "count")
                    .addAttribute("desc", "current item count")
                    .addAttribute("type", "int32")
                    .addAttribute("min", "0")
                    .addAttribute("max", String.valueOf(instances));
                e.addElement("attr")
                    .addAttribute("bin_id", "2")
                    .addAttribute("name", "shrink")
                    .addAttribute("desc", "shrink existing array")
                    .addAttribute("type", "boolean")
                    .addAttribute("default", "true");
            }
        }

        return e;
    }

    protected final org.dom4j.Element addChildren(org.dom4j.Element e) {
        for (Item item: items) {
            final int id = items.indexOf(item);

            e.add(item.asElement(id));
        }

        return e;
    }

    public org.dom4j.Element asElement(Integer id) {
        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("element")
            .addAttribute("name", name)
            .addAttribute("desc", getRciDescription())
            .addAttribute("type", "list")
            .addAttribute("bin_id", id.toString());

        addAttributes(e);
        addChildren(e);

        return wrapConditional(e);
    }

    public String getName() {
        return name;
    }

    public String getDescription() {
        return description;
    }

    public String getHelpDescription() {
        return helpDescription;
    }

    public boolean isDictionary() {
        return (instances == null);
    }

    public boolean isArray() {
        return !isDictionary();
    }

    public void setCapacity(Capacity capacity) {
        this.capacity = capacity;
    }

    public Capacity getCapacity() {
        return capacity;
    }

    public void setKeys(LinkedHashSet<String> keys) {
        this.keys = keys;
    }

    public LinkedHashSet<String> getKeys() {
        return (keys != null) ? keys : new LinkedHashSet<String>();
    }

    public void setInstances(int count) throws Exception {
        if (count <= 0) {
            throw new Exception("Invalid instance count for: " + name);
        }
        instances = count;
    }

    public Integer getInstances() {
        return instances;
    }

    public LinkedList<Item> getItems() {
        return items;
    }

    public void addItem(Item newItem) throws NamingException {
        String newName = newItem.getName();

        for (Item existingItem : items) {
            if (existingItem.getName().equals(newName)) {
                throw new NamingException("Duplicate name: " + newName);
            }
        }
        items.add(newItem);
    }

    public void validate(Config config) throws Exception {
        if (instances == null) { // no count specified
            if (keys == null) { // and no keys specified
                instances = 1;
                assert capacity == null;
                capacity = Capacity.FIXED;
            } else {
                if (capacity == null) {
                    capacity = keys.isEmpty() ? Capacity.VARIABLE : Capacity.FIXED;
                }
            }
        } else {
            if (keys != null) {
                throw new Exception("Count and keys both specified");
            }

            if (capacity == null) {
                capacity = Capacity.FIXED;
            }
        }

        if (keys != null) { // is dictionary
            assert capacity == Capacity.VARIABLE : "fixed dictionaries are not supported";
            if (config.getMaxDynamicKeyLength() == 0) {
                throw new Exception("max key length must be specified on the command line when using dynamic dictionaries");
            }
        }

        if (items.isEmpty()) {
            throw new Exception("No items specified");
        }
    }

    public Item find(final String name) throws NoSuchElementException {
        for (Item item: items) {
            if (item.name.equals(name)) {
                return item;
            }
        }
        throw new NoSuchElementException();
    }
}

package com.digi.connector.config;

import java.io.IOException;

public class Value extends Item {

    public Value(String name, String description, String helpDescription) throws IOException {
        super(name, description == null ? "" : description, helpDescription);
    }

    public org.dom4j.Element asElement(Integer id) {
        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("value")
            .addAttribute("value", name)
            .addAttribute("desc", getDescription());

        if (id != null) {
            e.addAttribute("bin_id", id.toString());
        }

        return e;
    }
}

package com.digi.connector.config;

import java.io.IOException;

public class Reference extends Item {

    public Reference(String name, String description, String helpDescription) throws IOException {
        super(name, description == null ? "" : description, helpDescription);
    }

    public org.dom4j.Element asElement(Integer id) {
        assert id == null;

        org.dom4j.Element e = org.dom4j.DocumentHelper.createElement("ref")
            .addAttribute("name", name)
            .addAttribute("desc", getDescription());

        return e;
    }
}

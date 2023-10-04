package com.digi.connector.config;

import java.util.EnumSet;

public class GenFsmFullHeaderFile extends GenFsmHeaderFile {
    public GenFsmFullHeaderFile() throws Exception {
        super(EnumSet.allOf(Element.Type.class), true, true, true, true, true);
    }
}

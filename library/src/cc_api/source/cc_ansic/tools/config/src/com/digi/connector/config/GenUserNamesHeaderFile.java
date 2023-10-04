package com.digi.connector.config;

import java.io.IOException;

import com.digi.connector.config.ConfigGenerator.ItemType;

public class GenUserNamesHeaderFile extends GenHeaderFile {

    private static String FILE = "rci_usenames_defines.h";

    public GenUserNamesHeaderFile() throws IOException {
        super(FILE, GenFile.Type.USER, GenFile.UsePrefix.CUSTOM);
    }

    private void generateUseNamesHeader(String type, int amount) throws IOException {
        write(String.format(
            "#if !(defined RCI_%s_NAME_MAX_SIZE)\n" +
            "#define RCI_%s_NAME_MAX_SIZE %d\n" +
            "#else\n" +
            "#if RCI_%s_NAME_MAX_SIZE < %d\n" +
            "#undef RCI_%s_NAME_MAX_SIZE\n" +
            "#define RCI_%s_NAME_MAX_SIZE %d\n" +
            "#endif\n" +
            "#endif\n", type, type, amount, type, amount, type, type, amount));
    }

    public void writeGuardedContent() throws Exception {
        if (options.useNames().contains(ItemType.ELEMENTS)) {
            generateUseNamesHeader("ELEMENTS", config.getMaxNameLengthSeen(ItemType.ELEMENTS) + 1);
        }

        if (options.useNames().contains(ItemType.COLLECTIONS)) {
            generateUseNamesHeader("COLLECTIONS", config.getMaxNameLengthSeen(ItemType.COLLECTIONS) + 1);
        }

        if (options.useNames().contains(ItemType.VALUES)) {
            generateUseNamesHeader("VALUES", config.getMaxNameLengthSeen(ItemType.VALUES) + 1);
        }
    }
}

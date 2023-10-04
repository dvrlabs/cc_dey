package com.digi.connector.config;

import java.util.Arrays;
import java.util.Stack;
import java.util.Vector;

public class Location {
    private Group.Type type;
    private Stack<String> data = new Stack<>();

    public Location(final Location other) {
        this.type = other.getType();
        this.data = new Stack<String>();
        this.data.addAll(other.getPath());
    }

    public Location(final Group.Type type) {
        this.type = type;
    }

    public Location(final String location) throws Exception {
        String path;

        if (location.contains(":")) {
            String array[] = location.split(":", 2);

            type = Group.Type.toType(array[0]);
            path = array[1];
        } else {
            type = Group.Type.SETTING; // default is setting as it is the most common
            path = location;
        }

        if (path.startsWith("/")) {
            path = path.substring(1);
        }

        data.addAll(Arrays.asList(path.split("/")));
    }

    public void descend(String name) {
        data.push(name);
    }

    public void ascend() {
        data.pop();
    }

    public Group.Type getType() {
        return type;
    }

    public Vector<String> getPath() {
        return new Vector<>(data);
    }

    public String relativeTo(final Location anchor) throws Exception {
        // Determines how to get to data from the POV of anchor - returned as relative path

        if (type != anchor.getType()) {
            throw new Exception("Unable to make relative path: setting/state mismatch");
        }

        if (data.equals(anchor.data)) {
            throw new Exception("Unable to make relative path: common object");
        }

        int common = 0;
        final int size = data.size();
        final int anchor_size = anchor.data.size();
        final int smallest = Math.min(size, anchor_size);
        for (int i = 0; i < smallest; i++) {
            if (!data.get(i).equals(anchor.data.get(i))) {
                break;
            }
            common += 1;
        }

        Vector<String> relative = new Vector<>();
        if (common == 0) {
            // so that we get a "/" at the start of the string
            relative.add("");
        } else {
            int up = anchor_size - common;
            while (up > 0) {
                relative.add("..");
                up -= 1;
            }
        }
        int last = size - 1;
        for (; common < size; common++) {
            relative.add(data.get(common));
            if (common != last) {
                relative.add("1");
            }
        }
        return String.join("/", relative);
    }
}

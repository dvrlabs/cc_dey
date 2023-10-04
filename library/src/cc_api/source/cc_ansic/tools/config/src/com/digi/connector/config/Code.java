package com.digi.connector.config;

import java.util.List;
import java.util.Map;
import java.util.LinkedList;

public final class Code {
    private static final String QUOTE = Character.toString('"');
    public static final String commented(String string) { return "/* " + string + " */"; }
    public static final String quoted(String string) { return QUOTE + string + QUOTE; }
    public static final String indented(String line) { return "    " + line; }

    public static final LinkedList<String> indented(List<String> lines) {
        LinkedList<String> result = new LinkedList<>();

        for (String line: lines) {
            result.add(indented(line));
        }
        return result;
    }

    public static final LinkedList<String> commas(List<String> strings) {
        LinkedList<String> result = new LinkedList<>();

        if (!strings.isEmpty()) {
            int last = strings.size() -1;
            for (String line: strings.subList(0, last)) {
                assert !line.isEmpty();
                result.add(line + ",");
            }
            result.add(strings.get(last));
        }
        return result;
    }

    private static final LinkedList<String> _struct(String typedef, String tag, List<String> fields, String type) {
        LinkedList<String> result = new LinkedList<>();

        assert tag.trim() == tag;
        if (!tag.isEmpty()) {
            tag = tag + " ";
        }
        result.add(typedef + "struct " + tag + "{");
        for (String field: fields) {
            result.add(indented(field + ";"));
        }
        assert type.trim() == type;
        if (!type.isEmpty()) {
            type = " " + type;
        }
        result.add("}" + type + ";");
        return result;
    }

    public static final LinkedList<String> structTaggedTypedef(String tag, List<String> fields, String type) { return _struct("typedef ", tag, fields, type); }
    public static final LinkedList<String> structTagged(String tag, List<String> fields) { return _struct("", tag, fields, ""); }
    public static final LinkedList<String> structTypedef(List<String> fields, String type) { return structTaggedTypedef("", fields, type); }
    public static final LinkedList<String> struct(List<String> fields) { return structTagged("", fields); }

    public static final String define(String name, String value) {
        assert !name.isEmpty();
        String result = "#define " + name;
        if (value != null) {
            assert !value.isEmpty();
            result += " " + value;
        }
        return result;
    }

    public static final String include(String filename) { return "#include " + quoted(filename); }

    public static final String define(String name) { return define(name, null); }
    public static final LinkedList<String> define(List<String> names) {
        LinkedList<String> result = new LinkedList<>();

        for (String name: names) {
            result.add(define(name));
        }
        return result;
    }

    public static final LinkedList<String> define(Map<String, Object> pairs) {
        LinkedList<String> result = new LinkedList<>();

        for (Map.Entry<String, Object> pair : pairs.entrySet()) {
            String name = pair.getKey();
            Object value = pair.getValue();

            result.add(define(name, value.toString()));
        }
        return result;
    }

    private static final LinkedList<String> _enumeration(String typedef, String tag, String type, Map<String, Integer> pairs) {
        LinkedList<String> result = new LinkedList<>();

        assert tag.trim() == tag;
        if (!tag.isEmpty()) {
            tag = tag + " ";
        }
        result.add(typedef + "enum " + tag + "{");

        LinkedList<String> lines = new LinkedList<>();
        int expected = 0;
        for (Map.Entry<String, Integer> pair : pairs.entrySet()) {
            String name = pair.getKey();
            Integer value = pair.getValue();
            String line = name;

            if (value != expected) {
                line += " = " + value;
            }
            expected = value + 1;

            lines.add(indented(line));
        }
        result.addAll(commas(lines));

        assert type.trim() == type;
        if (!type.isEmpty()) {
            type = " " + type;
        }
        result.add("}" + type + ";");
        return result;
    }

    public static final LinkedList<String> enumerationTaggedTypedef(String tag, String type, Map<String, Integer> pairs) { return _enumeration("typedef ", tag, type, pairs); }
    public static final LinkedList<String> enumerationTagged(String tag, Map<String, Integer> pairs) { return _enumeration("", tag, "", pairs); }
    public static final LinkedList<String> enumerationTypedef(String type, Map<String, Integer> pairs) { return enumerationTaggedTypedef("", type, pairs); }
    public static final LinkedList<String> enumeration(Map<String, Integer> pairs) { return enumerationTagged("", pairs); }

    public static final String parameters(String ...parameters) {
        return String.join(", ", parameters);
    }

    public static final String signature(String type, String name, String parameters) {
        return type + " " + name + "(" + parameters + ")";
    }

    public static final LinkedList<String> function(String signature, List<String> statements) {
        LinkedList<String> result = new LinkedList<>();

        result.add(signature);
        result.add("{");
        for (String statement: statements) {
            result.add(indented(statement + ";"));
        }
        result.add("}");
        return result;
    }

    public static final LinkedList<String> function(String type, String name, String parameters, List<String> statements) {
        return function(signature(type, name, parameters), statements);
    }

    public static final String prototype(String signature) {
        return signature + ";";
    }

    public static final String prototype(String type, String name, String parameters) {
        return prototype(signature(type, name, parameters));
    }

    public static final LinkedList<String> prototypes(List<String> signatures) {
        LinkedList<String> result = new LinkedList<>();

        for (String signature: signatures) {
            result.add(prototype(signature));
        }
        return result;
    }

    public static final LinkedList<String> prototypes(String type, List<String> names, String parameters) {
        LinkedList<String> result = new LinkedList<>();

        for (String name: names) {
            result.add(prototype(type, name, parameters));
        }
        return result;
    }

    public static final class Type {
        private String type;

        private Type(String base) { type = base; }

        public static final Type base(String base) { return new Type(base); }
        public static final Type struct(String tag) { return new Type("struct " + tag); }

        public final Type constant() { return new Type(type + " const"); }
        public final Type pointer() { return new Type(type + " *"); }
        public final String named(String name) { return type + " " + name; }
    }
}

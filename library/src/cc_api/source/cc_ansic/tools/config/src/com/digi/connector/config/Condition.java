package com.digi.connector.config;

public class Condition {
    public enum Operation {
        EQUALS, REGEX;

        public static Operation defaultOperation() {
            return EQUALS;
        }

        public static Operation toOperation(String str) throws Exception {
            try {
                return valueOf(str.toUpperCase());

            } catch (Exception e) {
                throw new Exception("Invalid <condition> operation: " + str);
            }
        }

        public String toString() {
            return name().toLowerCase();
        }
    }

    public enum RegexCase {
        MATCH, IGNORE;

        public static RegexCase defaultRegexCase() {
            return MATCH;
        }

        public static RegexCase toRegexCase(String str) throws Exception {
            try {
                return valueOf(str.toUpperCase());

            } catch (Exception e) {
                throw new Exception("Invalid condition Type: " + str);
            }
        }

        public String toString() {
            return name().toLowerCase();
        }
    }
    private Operation type;
    private String name;
    private Location source;
    private RegexCase regexCase;
    private String content; // holds either pattern on value

    private Condition(final Operation type, final String name, final Location source, final String content) {
        this.type = type;
        this.name = name;
        this.source = new Location(source);
        this.content = content;
    }

    public Condition(final String name, final Location source, final String value) {
        this(Operation.EQUALS, name, source, value);
    }

    public Condition(final String name, final Location source, final String pattern, final Condition.RegexCase regexCase) {
        this(Operation.REGEX, name, source, pattern);
        this.regexCase = regexCase;
    }

    public String getName() {
        return name;
    }

    public Location getLocation() {
        return source;
    }

    public org.dom4j.Element wrapper(final Location current) throws Exception {
        // <conditional type="reference" name="../../ip_version" value="ipv4">
        // <conditional type="reference" name="../protocol" operation="regex" regex_pattern="(auto|chap|pap)">
        org.dom4j.Element wrapper = org.dom4j.DocumentHelper.createElement("conditional")
            .addAttribute("type", "reference")
            .addAttribute("name", source.relativeTo(current))
            .addAttribute("operation", type.toString());

        switch (type) {
        case EQUALS:
            wrapper.addAttribute("value", content);
            break;
        case REGEX:
            wrapper.addAttribute("regex_pattern", content);
            wrapper.addAttribute("regex_case", regexCase.toString());
            break;
        }

        return wrapper;
    }
}

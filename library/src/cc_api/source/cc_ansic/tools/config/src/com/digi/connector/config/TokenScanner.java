package com.digi.connector.config;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.Scanner;
import java.util.regex.Pattern;

public class TokenScanner {

    private int lineNumber;
    private Scanner lineScanner;
    private Scanner tokenScanner;
    private String back;

    private final static ConfigGenerator options = ConfigGenerator.getInstance();

    public TokenScanner(String fileName) throws Exception {
        try {

            File configFile = new File(fileName);

            if (!configFile.exists()) {
                throw new NullPointerException(fileName + " file not found.");
            }

            lineScanner = new Scanner(new FileReader(configFile));
            tokenScanner = null;
            back = null;

        } catch (NullPointerException e) {
            options.log("Unable to open " + fileName);
            throw new Exception("Error in opening " + fileName);
        }
    }

    public void pushbackToken(String token) {
        assert back == null;

        back = token;
    }

    public String getToken() {
        String aWord;

        if (back == null) {
            aWord = null;
        } else {
            aWord = back;
            back = null;
        }

        while (aWord == null) {
            if ((tokenScanner != null) && (!tokenScanner.hasNext())) {
                tokenScanner.close();
                tokenScanner = null;
            }
            if (tokenScanner == null) {
                while (lineScanner.hasNextLine()) {
                    String line = lineScanner.nextLine();
                    lineNumber++;
                    // ConfigGenerator.log("line " + lineNumber + ": " + line);
                    if ((line.length() > 0) && (line.split(" ").length > 0)) {
                        tokenScanner = new Scanner(line);
                        break;
                    }
                }
            }

            if ((tokenScanner != null) && (tokenScanner.hasNext())) {
                aWord = tokenScanner.next();
            }

            if (!lineScanner.hasNextLine()) {
                break;
            }
        }

        return aWord;
    }

    public int getTokenInt() throws IOException {
        String str = getToken();

        try {
            return Integer.parseInt(str);
        } catch (NumberFormatException e) {
            throw new IOException("Not an integer (expect an integer value)");
        }
    }

    public String getTokenInLine(String pattern) {
        String aLine = null;

        if ((tokenScanner != null) && (!tokenScanner.hasNext())) {
            tokenScanner.close();
            tokenScanner = null;
        }
        if (tokenScanner == null) {
            while (lineScanner.hasNextLine()) {
                String line = lineScanner.nextLine();
                lineNumber++;
                // ConfigGenerator.log("line " + lineNumber + ": " + line);
                if ((line.length() > 0) && (line.split(" ").length > 0)) {
                    tokenScanner = new Scanner(line);
                    break;
                }
            }
        }

        if (tokenScanner.hasNext())
            aLine = tokenScanner.findInLine(pattern);

        return aLine;
    }

    public boolean hasToken() {
        if (back != null) {
            return true;
        }

        if ((tokenScanner != null) && tokenScanner.hasNext()) {
            return true;
        }

        if (lineScanner.hasNext()) {
            return true;
        }

        return false;
    }

    public boolean hasToken(String pattern) {
        if ((back != null) && Pattern.matches(pattern, back)) {
            return true;
        }

        if ((tokenScanner != null) && tokenScanner.hasNext(pattern)) {
            return true;
        }

        if (lineScanner.hasNext(pattern)) {
            return true;
        }

        return false;
    }

    public boolean hasTokenInt() {
        if (back != null) {
            int radix = (tokenScanner != null) ? tokenScanner.radix() : lineScanner.radix();

            try {
                Integer.parseInt(back, radix);
                return true;
            } catch (NumberFormatException e) {
                return false;
            }
        }

        if ((tokenScanner != null) && tokenScanner.hasNextInt()) {
            return true;
        }

        if (lineScanner.hasNextInt()) {
            return true;
        }

        return false;
    }

    public int getLineNumber() {
        return lineNumber;
    }

    public void skipCommentLine() {
        tokenScanner.close();
        tokenScanner = null;
    }

    public void close() {
        // ensure the underlying stream is always closed
        // this only has any effect if the item passed to the Scanner
        // constructor implements Closeable (which it does in this case).
        if (tokenScanner != null) {
            tokenScanner.close();
        }
        lineScanner.close();

    }

}

public class FormatUtils {
    static public String firstLetterToLowercase(String s) {
        if (s.length() == 0) {
            return s;
        }
        return Character.toLowerCase(s.charAt(0)) + s.substring(1);
    }

    static public String firstLetterToUppercase(String s) {
        if (s.length() == 0) {
            return s;
        }
        return Character.toUpperCase(s.charAt(0)) + s.substring(1);
    }

    static public String camelCaseToUpperSnakeCase(String s) {
        return transformCamelCase(s, Character::toUpperCase, "_");
    }

    static public String camelCaseToFirstLetterUppercaseWithSpaces(String s) {
        return transformCamelCase(s, c -> c, Character::toUpperCase, " ");
    }

    static public String camelCaseToLowercaseWithSpaces(String s) {
        return transformCamelCase(s, Character::toLowerCase, " ");
    }

    interface CharTransformer {
        char run(char c);
    }

    static String transformCamelCase(String s, CharTransformer transformer, String separator) {
        return transformCamelCase(s, transformer, transformer, separator);
    }

    static String transformCamelCase(String s, CharTransformer transformer, CharTransformer startOrAfterSeparatorCharTransformer, String separator) {
        String result = "";
        var previousCharIsDigit = false;
        for (int i = 0; i < s.length(); i++) {
            var c = s.charAt(i);

            var isAfterSeparator = false;

            if ((Character.isUpperCase(c) || (Character.isDigit(c) && !previousCharIsDigit)) && i != 0) {
                result += separator;
                isAfterSeparator = true;
            }

            var isFirstChar = i == 0;
            if (isFirstChar || isAfterSeparator) {
                result += startOrAfterSeparatorCharTransformer.run(c);
            } else {
                result += transformer.run(c);
            }

            previousCharIsDigit = Character.isDigit(c);
        }
        return result;
    }
}

import java.util.Optional;

enum TokenType {
    IDENTIFIER,
    LEFT_BRACE,
    RIGHT_BRACE,
    LESS_THAN,
    MORE_THAN,
    DOUBLE_LEFT_BRACKET,
    DOUBLE_RIGHT_BRACKET,
    COMMA,
    SEMICOLON,
    EQUALS,
    CPP,
    CPP_TYPE,
    INT,
    FLOAT
}

class Token {
    TokenType type;
    int start;
    int length;
    String text;

    Token(TokenType type, int start, int length, String source) {
        this.type = type;
        this.start = start;
        this.length = length;
        this.text = source.substring(start, start + length);
    }

    String cppSource() {
        if (type != TokenType.CPP)
            return "";
        return text.substring(1, text.length() - 1);
    }

    String cppType() {
        if (type != TokenType.CPP_TYPE)
            return "";
        return text.substring(1, text.length() - 1);
    }

    long intValue() {
        return Long.parseLong(text);
    }

    float floatValue() {
        return Float.parseFloat(text);
    }
}

class LexerError extends Exception {

}

public class Lexer {
    String source;
    int tokenStartIndex = 0;
    int currentIndex = 0;

    Lexer(String source) {
        this.source = source;
    }

    boolean isIdentifierStartChar(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_') || (c == '$');
    }

    boolean isDigit(char c) {
        return (c >= '0') && (c <= '9');
    }

    public Optional<Token> nextToken() throws LexerError {
        if (source.length() == 0)
            return Optional.empty();

        skipWhitespace();

        if (isAtEnd())
            return Optional.empty();

        var c = peek();
        eat();
        switch (c) {
            case '{': return Optional.of(makeToken(TokenType.LEFT_BRACE));
            case '}': return Optional.of(makeToken(TokenType.RIGHT_BRACE));
            case '<': return Optional.of(makeToken(TokenType.LESS_THAN));
            case '>': return Optional.of(makeToken(TokenType.MORE_THAN));
            case ',': return Optional.of(makeToken(TokenType.COMMA));
            case ';': return Optional.of(makeToken(TokenType.SEMICOLON));
            case '=': return Optional.of(makeToken(TokenType.EQUALS));
            case '[':
                if (match('[')) {
                    return Optional.of(makeToken(TokenType.DOUBLE_LEFT_BRACKET));
                }
                break;
            case ']':
                if (match(']')) {
                    return Optional.of(makeToken(TokenType.DOUBLE_RIGHT_BRACKET));
                }
                break;
            case '`':
                while (!isAtEnd()) {
                    if (match('`')) {
                        return Optional.of(makeToken(TokenType.CPP));
                    }
                    eat();
                }
                break;

            case '$':
                while (!isAtEnd()) {
                    if (match('$')) {
                        return Optional.of(makeToken(TokenType.CPP_TYPE));
                    }
                    eat();
                }
                break;

            default:
                if (isIdentifierStartChar(c)) {
                    while (!isAtEnd() && (isIdentifierStartChar(peek()) || isDigit(peek())))
                        eat();

                    Token token = makeToken(TokenType.IDENTIFIER);
                    return Optional.of(token);
                } else if (isDigit(c)) {
                    return number(c);
                } else if (c == '-') {
                    return number(peek());
                }
        }
        throw new LexerError();
    }

    Optional<Token> number(char firstDigit) throws LexerError {
        if (firstDigit == '0') {
            return Optional.of(makeToken(TokenType.INT));
        }

        var afterDot = false;
        while (!isAtEnd()) {
            if (!afterDot && match('.')) {
                afterDot = true;
            } else if (!isDigit(peek())) {
                if (afterDot) {
                    return Optional.of(makeToken(TokenType.FLOAT));
                } else {
                    return Optional.of(makeToken(TokenType.INT));

                }
            }
            eat();
        }
        throw new LexerError();
    }

    void skipWhitespace() {
        while (!isAtEnd()) {
            switch (peek()) {
                case ' ', '\t', '\r', '\f', '\n' -> eat();
                case '/' -> {
                    if (peekNext() == '/') {
                        while (!isAtEnd() && peek() != '\n')
                            eat();
                    }
                }
                default -> {
                    tokenStartIndex = currentIndex;
                    return;
                }
            }
        }
    }

    Token makeToken(TokenType type) {
	    Token token = new Token(type, tokenStartIndex, currentIndex - tokenStartIndex, source);
        tokenStartIndex = currentIndex;

        return token;
    }

    void eat()  {
        if (!isAtEnd())
            currentIndex++;
    }

    boolean match(char c) {
        if (peek() == c) {
            eat();
            return true;
        }
        return false;
    }

    char peek() {
        return source.charAt(currentIndex);
    }

    char peekNext() {
        if (currentIndex + 1 >= source.length())
            return '\0';
        return source.charAt(currentIndex + 1);
    }

    boolean isAtEnd() {
        return currentIndex >= source.length();
    }
}

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

class ParserError extends Exception {
    ParserError(String message) {
        super(message);
    }
}

public class Parser {
    String source;
    Lexer lexer;

    boolean isAtEnd;
    Token currentToken;
    Token previousToken;

    DataFile output = new DataFile();

    Parser(String source) {
        this.source = source;
        this.lexer = new Lexer(source);
        isAtEnd = false;
    }

    DataFile parse() throws LexerError, ParserError {

        var optToken = lexer.nextToken();
        if (optToken.isEmpty()) {
            return output;
        }
        this.currentToken = optToken.get();

        while (!isAtEnd) {
            output.declarations.add(declaration());
        }

        return output;
    }

    Declaration declaration() throws LexerError, ParserError {
        if (matchIdentifier("struct")) {
            List<StructAttribute> attributes = new ArrayList<>();

            if (match(TokenType.DOUBLE_LEFT_BRACKET)) {
                do {
                    attributes.add(structAttribute());
                } while (match(TokenType.COMMA));
                expect(TokenType.DOUBLE_RIGHT_BRACKET);
            }

            expect(TokenType.IDENTIFIER);
            var name = previousToken.text;

            List<DeclarationInStruct> declarations = new ArrayList<>();

            expect(TokenType.LEFT_BRACE);
            while (!isAtEnd && !check(TokenType.RIGHT_BRACE)) {
                declarations.add(declarationInStruct());
            }
            expect(TokenType.RIGHT_BRACE);

            return new Struct(name, declarations, attributes);
        } else if (matchIdentifier("enum")) {
            List<EnumAttribute> attributes = new ArrayList<>();

            if (match(TokenType.DOUBLE_LEFT_BRACKET)) {
                do {
                    attributes.add(enumAttribute());
                } while (match(TokenType.COMMA));
                expect(TokenType.DOUBLE_RIGHT_BRACKET);
            }

            expect((TokenType.IDENTIFIER));
            var name = previousToken.text;

            List<EnumDefinition> definitions = new ArrayList<>();
            expect(TokenType.LEFT_BRACE);

            if (match(TokenType.RIGHT_BRACE)) {
                return new Enum(name, definitions, attributes);
            }

            for (;;) {
                expect(TokenType.IDENTIFIER);
                var key = previousToken.text;
                Optional<String> value = Optional.empty();
                if (match(TokenType.EQUALS)) {
                    // Maybe instead of cpp tokens skip untill there is a comma and then add the string together. This will not always work but might be better. Macros might break it or constexpr functions.
                    expect(TokenType.CPP);
                    value = Optional.of(previousToken.cppSource());
                }
                definitions.add(new EnumDefinition(key, value));

                if (match(TokenType.RIGHT_BRACE)) {
                    return new Enum(name, definitions, attributes);
                }
                expect(TokenType.COMMA);

                if (match(TokenType.RIGHT_BRACE)) {
                    return new Enum(name, definitions, attributes);
                }
            }
        }

        throw new ParserError("expected declaration");
    }

    StructAttribute structAttribute() throws LexerError, ParserError {
        if (matchIdentifier("NetworkSerialize")) {
            output.addIncludePath(Config.NETWORKING_PATH);
            return new StructAttributeNetworkSerialize();
        } else if (matchIdentifier("Gui")) {
            output.addCppIncludePath(Config.GUI_PATH);
            return new StructAttributeGui();
        } else if (matchIdentifier("Json")) {
            output.addIncludePath(Config.JSON_PATH);
            return new StructAttributeJson();
        }
        throw new ParserError("expected struct attribute");
    }

    EnumAttribute enumAttribute() throws LexerError, ParserError {
        if (matchIdentifier("ToStr")) {
            return new EnumAttributeToStr();
        } else if (matchIdentifier("ImGuiCombo")) {
            return new EnumAttributeImGuiCombo();
        }
        throw new ParserError("expected enum attribute");
    }

    DeclarationInStruct declarationInStruct() throws LexerError, ParserError {
        if (match(TokenType.CPP)) {
            return new CppInStruct(previousToken.cppSource());
        } else {
            var dataType = dataType();
            expect(TokenType.IDENTIFIER);
            var name = previousToken.text;

            Optional<String> defaultValue = Optional.empty();
            if (match(TokenType.EQUALS)) {
                expect(TokenType.CPP);
                defaultValue = Optional.of(previousToken.cppSource());
            }

            expect(TokenType.SEMICOLON);
            return new Field(dataType, name, defaultValue);
        }
    }

    DataType dataType() throws LexerError, ParserError {
        if (matchIdentifier("float")) {
            return new FloatDataType();
        } else if (matchIdentifier("i32")) {
            output.addIncludePath(Config.TYPES_PATH);
            return new I32DataType();
        } else if (matchIdentifier("bool")) {
            return new BoolDataType();
        } else if (matchIdentifier("color")) {
            output.addIncludePath(Config.VEC4_PATH);
            return new ColorDataType();
        } else if (matchIdentifier("ranged_int")) {
            expect(TokenType.LESS_THAN);

            expect(TokenType.INT);
            var min = previousToken.intValue();

            expect(TokenType.COMMA);

            expect(TokenType.INT);
            var max = previousToken.intValue();

            expect(TokenType.MORE_THAN);
            output.addIncludePath(Config.TYPES_PATH);
            return new RangedSignedIntDataType(min, max);
        } else if (matchIdentifier("vector")) {
            expect(TokenType.LESS_THAN);
            var itemDataType = dataType();
            expect(TokenType.MORE_THAN);
            return new VectorDataType(itemDataType);
        } else if (matchIdentifier("ranged_float")) {
            expect(TokenType.LESS_THAN);

            expectNumber();
            var min = previousToken.floatValue();

            expect(TokenType.COMMA);

            expectNumber();
            var max = previousToken.floatValue();
            expect(TokenType.MORE_THAN);
            return new RangedFloatDataType(min, max);
        } else {
            expect(TokenType.IDENTIFIER);
            return new IdentifierDataType(previousToken.text);
        }
    }

    void eat() throws LexerError {
        if (isAtEnd)
            return;

        var next = lexer.nextToken();
        if (next.isEmpty()) {
            isAtEnd = true;
            return;
        }
        previousToken = currentToken;
        currentToken = next.get();
    }

    boolean matchIdentifier(String text) throws LexerError {
        if (currentToken.type == TokenType.IDENTIFIER && currentToken.text.equals(text)) {
            eat();
            return true;
        }
        return false;
    }

    boolean match(TokenType type) throws LexerError {
        if (currentToken.type == type) {
            eat();
            return true;
        }
        return false;
    }

    boolean check(TokenType type) {
        return currentToken.type == type;
    }

    void expect(TokenType type) throws LexerError, ParserError {
        // This must be the order. First check if isAtEnd then do match, because match can change isAtEnd
        if (isAtEnd || !match(type)) {
            throw new ParserError("expected");
        }
    }

    void expectNumber() throws LexerError, ParserError {
        if (isAtEnd || (!match(TokenType.FLOAT) && !match(TokenType.INT))) {
            throw new ParserError("expected number");
        }
    }

}
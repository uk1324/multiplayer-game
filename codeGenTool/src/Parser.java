import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Optional;

class ParserError extends Exception {
    ParserError(String message) {
        super(message);
    }
    ParserError(String message, int line) {
        super(message);
        this.line = Optional.of(line);
    }
    Optional<Integer> line = Optional.empty();
}

public class Parser {
    String source;
    GeneratedFilesPaths paths;

    Lexer lexer;

    boolean isAtEnd;
    Token currentToken;
    Token previousToken;

    DataFile output = new DataFile();

    Parser(String source, GeneratedFilesPaths paths) {
        this.source = source;
        this.paths = paths;
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

    List<Field> filterKeepOnlyFields(List<DeclarationInStruct> declarations) {
        List<Field> fields = new ArrayList<>();
        for (var declaration : declarations) {
            if (declaration instanceof Field) {
                fields.add((Field)declaration);
            }
        }
        return fields;
    }

    Declaration declaration() throws LexerError, ParserError {
        if (matchIdentifier("struct")) {
            var attributes = structAttributes();
            expect(TokenType.IDENTIFIER);
            var name = previousToken.text;
            var declarations = structDeclarationsBlock();
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
                    // Maybe instead of cpp tokens skip until there is a comma and then add the string together. This will not always work but might be better. Macros might break it or constexpr functions.
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
        } else if (matchIdentifier("shader")) {
            return shader();
        } else if (match(TokenType.CPP)) {
            return new Cpp(previousToken.cppSource());
        }
        throw new ParserError("expected declaration");
    }

    List<StructAttribute> structAttributes() throws ParserError, LexerError {
        List<StructAttribute> attributes = new ArrayList<>();
        if (match(TokenType.DOUBLE_LEFT_BRACKET)) {
            do {
                attributes.add(structAttribute());
            } while (match(TokenType.COMMA));
            expect(TokenType.DOUBLE_RIGHT_BRACKET);
        }
        return attributes;
    }

    List<DeclarationInStruct> structDeclarationsBlock() throws ParserError, LexerError {
        List<DeclarationInStruct> declarations = new ArrayList<>();

        expect(TokenType.LEFT_BRACE);
        while (!isAtEnd && !check(TokenType.RIGHT_BRACE)) {
            declarations.add(declarationInStruct());
        }
        expect(TokenType.RIGHT_BRACE);

        return declarations;
    }

    List<Field> structFieldsBlock() throws ParserError, LexerError {
        List<Field> declarations = new ArrayList<>();

        expect(TokenType.LEFT_BRACE);
        while (!isAtEnd && !check(TokenType.RIGHT_BRACE)) {
            declarations.add(structField());
        }
        expect(TokenType.RIGHT_BRACE);

        return declarations;
    }

    Struct structBody(String name) throws ParserError, LexerError {
        var attributes = structAttributes();
        var declarations = structDeclarationsBlock();
        return new Struct(name, declarations, attributes);
    }

    StructAttribute structAttribute() throws LexerError, ParserError {
        if (matchIdentifier("NetworkSerialize")) {
            output.addHppIncludePath(Config.NETWORKING_PATH);
            output.addHppIncludePath(Config.NETWORKING_UTILS_PATH);
            return new StructAttributeNetworkSerialize();
        } else if (matchIdentifier("Gui")) {
            output.addCppIncludePath(Config.GUI_PATH);
            return new StructAttributeGui();
        } else if (matchIdentifier("Json")) {
            output.addHppIncludePath(Config.JSON_PATH);
            output.addCppIncludePath(Config.JSON_UTILS_PATH);
            return new StructAttributeJson();
        } else if (matchIdentifier("NetworkMessage")) {
            output.addHppIncludePath(Config.NETWORKING_PATH);
            output.addHppIncludePath(Config.NETWORKING_UTILS_PATH);
            return new StructAttributeNetworkMessage();
        } else if (matchIdentifier("Bullet")) {
            return new StructAttributeBullet();
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

    Field structField() throws ParserError, LexerError {
        var dataType = dataType();

        List<FieldAttribute> attributes = new ArrayList<>();
        if (match(TokenType.DOUBLE_LEFT_BRACKET)) {
            do {
                attributes.add(fieldAttribute());
            } while (match(TokenType.COMMA));
            expect(TokenType.DOUBLE_RIGHT_BRACKET);
        }

        expect(TokenType.IDENTIFIER);
        var name = previousToken.text;

        Optional<String> defaultValue = Optional.empty();
        if (match(TokenType.EQUALS)) {
            expect(TokenType.CPP);
            defaultValue = Optional.of(previousToken.cppSource());
        }

        expect(TokenType.SEMICOLON);
        return new Field(dataType, name, defaultValue, attributes);
    }

    FieldAttribute fieldAttribute() throws LexerError, ParserError {
        if (matchIdentifier("NoNetworkSerialize")) {
            return new FieldAttributeNoNetworkSerialize();
        } else if (matchIdentifier("NoJsonSerialize")) {
            return new FieldAttributeNoJsonSerialize();
        } else {
            throw new ParserError("expected field attribute");
        }
    }

    DeclarationInStruct declarationInStruct() throws LexerError, ParserError {
        if (match(TokenType.CPP)) {
            return new CppInStruct(previousToken.cppSource());
        } else {
            return structField();
        }
    }

    Shader shader()  throws LexerError, ParserError {
        expect(TokenType.IDENTIFIER);
        var name = previousToken.text;
        expect(TokenType.LEFT_BRACE);

        String instanceStructName = name + "Instance";
        String vertUniformsStructName = name + "VertUniforms";
        String fragUniformsStructName = name + "FragUniforms";

        Optional<String> optVertexFormat = Optional.empty();
        Optional<List<StructAttribute>> optInstanceAttributes = Optional.empty();
        Optional<List<DeclarationInStruct>> optVertInstanceDeclarations = Optional.empty();
        Optional<List<DeclarationInStruct>> optFragInstanceDeclarations = Optional.empty();
        Optional<Struct> optVertUniforms = Optional.empty();
        Optional<Struct> optFragUniforms = Optional.empty();
        Optional<List<Field>> optVertOut = Optional.empty();

        boolean generateVert = true;

        while (!isAtEnd && !match(TokenType.RIGHT_BRACE)) {
            expect(TokenType.IDENTIFIER);
            if (previousToken.text.equals("vertInstance")) {
                if (optVertInstanceDeclarations.isPresent()) {
                    throw new ParserError("vertInstance already specified");
                }
                expect(TokenType.EQUALS);
                optVertInstanceDeclarations = Optional.of(structDeclarationsBlock());
                expect(TokenType.SEMICOLON);
            } else if (previousToken.text.equals("fragInstance")) {
                if (optFragInstanceDeclarations.isPresent()) {
                    throw new ParserError("fragInstance already specified");
                }
                expect(TokenType.EQUALS);
                optFragInstanceDeclarations = Optional.of(structDeclarationsBlock());
                expect(TokenType.SEMICOLON);
            } else if (previousToken.text.equals("instanceAttributes")) {
                if (optInstanceAttributes.isPresent()) {
                    throw new ParserError("instanceAttributes already specified");
                }
                expect(TokenType.EQUALS);
                optInstanceAttributes = Optional.of(structAttributes());
                expect(TokenType.SEMICOLON);
            }  else if (previousToken.text.equals("vertexFormat")) {
                if (optVertexFormat.isPresent()) {
                    throw new ParserError("vertexFormat already specified");
                }
                expect(TokenType.EQUALS);

                expect(TokenType.IDENTIFIER);
                optVertexFormat = Optional.of(previousToken.text);
                expect(TokenType.SEMICOLON);
            } else if (previousToken.text.equals("vertUniforms")) {
                if (optVertUniforms.isPresent()) {
                    throw new ParserError("vertUniforms already specified");
                }
                expect(TokenType.EQUALS);
                optVertUniforms = Optional.of(structBody(vertUniformsStructName));
                expect(TokenType.SEMICOLON);
            } else if (previousToken.text.equals("fragUniforms")) {
                if (optFragUniforms.isPresent()) {
                    throw new ParserError("fragUniforms already specified");
                }
                expect(TokenType.EQUALS);
                optFragUniforms = Optional.of(structBody(fragUniformsStructName));
                expect(TokenType.SEMICOLON);
            } else if (previousToken.text.equals("vertOut")) {
                if (optVertOut.isPresent()) {
                    throw new ParserError("vertOut already specified");
                }
                expect(TokenType.EQUALS);
                optVertOut = Optional.of(structFieldsBlock());
                expect(TokenType.SEMICOLON);
            } else if (previousToken.text.equals("fragOnly")) {
                expect(TokenType.SEMICOLON);
            } else if (previousToken.text.equals("generateVert")) {
                expect(TokenType.EQUALS);
                generateVert = bool();
                expect(TokenType.SEMICOLON);
            } else {
                throw new ParserError(String.format("invalid field %s", previousToken.text));
            }
        }

        String vertexFormat;
        if (optVertexFormat.isPresent()) {
            vertexFormat = optVertexFormat.get();
        } else if (generateVert) {
            throw new ParserError("vertexFormat must be specified");
        } else {
            // @Hack
            vertexFormat = "PT";
        }

        List<DeclarationInStruct> instanceDeclarations = new ArrayList<>();
        List<Field> instanceFragFields = new ArrayList<>();
        List<Field> instanceVertFields = new ArrayList<>();
        if (optVertInstanceDeclarations.isPresent()) {
            instanceDeclarations.addAll(optVertInstanceDeclarations.get());
            instanceVertFields = filterKeepOnlyFields(optVertInstanceDeclarations.get());
        }
        if (optFragInstanceDeclarations.isPresent()) {
            instanceDeclarations.addAll(optFragInstanceDeclarations.get());
            instanceFragFields = filterKeepOnlyFields(optFragInstanceDeclarations.get());
        }

        List<StructAttribute> instanceAttributes = optInstanceAttributes.orElse(new ArrayList<>());
        Struct instance = new Struct(instanceStructName, instanceDeclarations, instanceAttributes);

        Struct vertUniforms;
        vertUniforms = optVertUniforms.orElseGet(() -> Struct.empty(vertUniformsStructName));
        vertUniforms.attributes.add(new StructAttributeUniform());

        Struct fragUniforms;
        fragUniforms = optFragUniforms.orElseGet(() -> Struct.empty(fragUniformsStructName));
        fragUniforms.attributes.add(new StructAttributeUniform());

        List<Field> vertOut = optVertOut.orElse(new ArrayList<>());

        output.addHppIncludePath(Config.SHADER_PROGRAM_PATH);
        output.addHppIncludePath(new IncludePath("vector"));
        output.addHppIncludePath(Config.VAO_PATH);
        output.addCppIncludePath(Config.OPENGL_PATH);

        return new Shader(name, instance, fragUniforms, vertUniforms, vertexFormat, instanceVertFields, instanceFragFields, vertOut, paths, generateVert);
    }

    DataType dataType() throws LexerError, ParserError {
        if (matchIdentifier("float")) {
            return new FloatDataType();
        } else if (matchIdentifier("i32")) {
            output.addHppIncludePath(Config.TYPES_PATH);
            return new I32DataType();
        } else if (matchIdentifier("bool")) {
            return new BoolDataType();
        } else if (matchIdentifier("color")) {
            output.addHppIncludePath(Config.VEC4_PATH);
            return new ColorDataType();
        } else if (matchIdentifier("color3")) {
            output.addHppIncludePath(Config.VEC3_PATH);
            return new Color3DataType();
        } else if (matchIdentifier("ranged_int")) {
            expect(TokenType.LESS_THAN);

            expect(TokenType.CPP);
            var min = previousToken.cppSource();

            expect(TokenType.COMMA);

            expect(TokenType.CPP);
            var max = previousToken.cppSource();

            expect(TokenType.MORE_THAN);
            output.addHppIncludePath(Config.TYPES_PATH);
            return new RangedSignedIntDataType(min, max);
        } else if (matchIdentifier("vector")) {
            expect(TokenType.LESS_THAN);
            var itemDataType = dataType();
            expect(TokenType.MORE_THAN);
            return new VectorDataType(itemDataType);
        } else if (matchIdentifier("map")) {
            expect(TokenType.LESS_THAN);
            var key = dataType();
            expect(TokenType.COMMA);
            var value = dataType();
            expect(TokenType.MORE_THAN);
            return new MapDataType(key, value);
        } else if (matchIdentifier("ranged_float")) {
            expect(TokenType.LESS_THAN);

            expectNumber();
            var min = previousToken.floatValue();

            expect(TokenType.COMMA);

            expectNumber();
            var max = previousToken.floatValue();
            expect(TokenType.MORE_THAN);
            return new RangedFloatDataType(min, max);
        } else if (match(TokenType.IDENTIFIER)) {
            if (previousToken.text.equals("Vec2")) {
                output.addHppIncludePath(Config.VEC2_PATH);
            }
            return new IdentifierDataType(previousToken.text);
        } else if (match(TokenType.CPP_TYPE)) {
            return new IdentifierDataType(previousToken.cppType());
        }
        throw new ParserError("expected data type");
    }

    boolean bool() throws LexerError, ParserError {
        if (matchIdentifier("true")) {
            return true;
        } else if (matchIdentifier("false")) {
            return false;
        } else {
            throw new ParserError("expected boolean");
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

    int tokenLine(final Token token) {
        int line = 0;
        for (int i = 0; i < source.length(); i++) {
            if (source.charAt(i) == '\n') {
                line++;
            }
            if (i == token.start) {
                break;
            }
        }
        return line;
    }

    void expect(TokenType type) throws LexerError, ParserError {
        // This must be the order. First check if isAtEnd then do match, because match can change isAtEnd
        if (isAtEnd || !match(type)) {
            throw new ParserError(String.format("unexpected '%s'", currentToken.text), tokenLine(currentToken));
        }
    }

    void expectNumber() throws LexerError, ParserError {
        if (isAtEnd || (!match(TokenType.FLOAT) && !match(TokenType.INT))) {
            throw new ParserError("expected number");
        }
    }

}
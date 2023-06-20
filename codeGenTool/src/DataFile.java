import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Optional;

// Remember to set everything to public.

public class DataFile {
    public List<Declaration> declarations = new ArrayList<>();
    private List<IncludePath> includePaths = new ArrayList<>();
    private List<IncludePath> cppIncludePaths = new ArrayList<>();

    public List<IncludePath> getIncludePaths() {
        return includePaths;
    }

    public List<IncludePath> getCppIncludePaths() {
        return cppIncludePaths;
    }

    static private void addUnique(List<IncludePath> list, IncludePath newPath) {
        for (var path : list) {
            if (path.isRelative == newPath.isRelative && path.path == newPath.path) {
                return;
            }
        }
        list.add(newPath);
    }

    public void addIncludePath(IncludePath newPath) {
        addUnique(includePaths, newPath);
    }

    public void addCppIncludePath(IncludePath newPath) {
        addUnique(cppIncludePaths, newPath);
    }
}

abstract class Declaration {
    public boolean getIsStruct() {
        return this instanceof Struct;
    }
    public boolean getIsCpp() {
        return this instanceof Cpp;
    }
    public boolean getIsEnum() {
        return this instanceof Enum;
    }
    public boolean getIsShader() {
        return this instanceof Shader;
    }
}

class IncludePath {
    public boolean isRelative;
    public String path;

    IncludePath(String path, boolean isRelative) {
        this.path = path;
        this.isRelative = isRelative;
    }

    IncludePath(String path) {
        this(path, false);
    }
}

class Struct extends Declaration {
    public String name;
    public List<DeclarationInStruct> declarations;
    public List<StructAttribute> attributes;
    public List<Field> fields = new ArrayList<>();

    Struct(String name, List<DeclarationInStruct> declarations, List<StructAttribute> attributes) {
        this.name = name;
        this.declarations = declarations;
        this.attributes = attributes;

        for (var declaration : declarations) {
            if (declaration instanceof Field) {
                fields.add((Field)declaration);
            }
        }
    }

    public static Struct empty(String name) {
        return new Struct(name, new ArrayList<>(), new ArrayList<>());
    }


    public Iterator<Field> getFields() {
        var stream = declarations
                .stream()
                .filter((DeclarationInStruct declaration) -> declaration instanceof Field)
                .map((DeclarationInStruct declaration) -> (Field)declaration);
        return stream.iterator();

        // How to use iterables instead of iterators.
        // https://stackoverflow.com/questions/31702989/how-to-iterate-with-foreach-loop-over-java-8-stream
    }

    public String getNameUpperSnakeCase() {
        return FormatUtils.camelCaseToUpperSnakeCase(name);
    }

    public boolean getIsNetworkSerialize() {
        return attributes.stream().anyMatch(a -> a instanceof StructAttributeNetworkSerialize);
    }
    public boolean getIsGui() {
        return attributes.stream().anyMatch(a -> a instanceof StructAttributeGui);
    }
    public boolean getIsJson() {
        return attributes.stream().anyMatch(a -> a instanceof StructAttributeJson);
    }
    public boolean getIsNetworkMessage() {
        return attributes.stream().anyMatch(a -> a instanceof StructAttributeNetworkMessage);
    }
    public boolean getIsUniform() {
        return attributes.stream().anyMatch(a -> a instanceof StructAttributeUniform);
    }
}

abstract class StructAttribute { }

class StructAttributeNetworkSerialize extends StructAttribute { }
class StructAttributeNetworkMessage extends StructAttribute { }
class StructAttributeGui extends StructAttribute { }
class StructAttributeJson extends StructAttribute { }
class StructAttributeUniform extends StructAttribute { }

class Enum extends Declaration {
    public String name;
    public List<EnumDefinition> definitions;
    public List<EnumAttribute> attributes;

    Enum(String name, List<EnumDefinition> definitions, List<EnumAttribute> attributes) {
        this.name = name;
        this.definitions = definitions;
        this.attributes = attributes;
    }

    public boolean getIsToStr() {
        return attributes.stream().anyMatch(a -> a instanceof EnumAttributeToStr);
    }
    public boolean getIsImGuiCombo() {
        return attributes.stream().anyMatch(a -> a instanceof EnumAttributeImGuiCombo);
    }
}

class EnumDefinition {
    public String name;
    // If you are using things like toStr it should ignore the COUNT. Could automatically calculate count or max_value of the enum, but this would require evaulating the int expressions.
    public Optional<String> initializerCppSource;

    EnumDefinition(String name, Optional<String> initializerCppSource) {
        this.name = name;
        this.initializerCppSource = initializerCppSource;
    }

    public boolean getHasInitializer() {
        return initializerCppSource.isPresent();
    }

    public String getInitializer() {
        return initializerCppSource.get();
    }
}

class Shader extends Declaration {
    public String name;
    public Struct instance;
    public Struct fragUniforms;
    public Struct vertUniforms;
    public String vertexFormat;
    public List<VertexAttribute> vertexAttributes = new ArrayList<>();
    public List<Field> instanceFragFields;
    public List<Field> instanceVertFields;
    public List<Field> vertOut;

    public boolean getVertUniformsIsEmpty() {
        return vertUniforms.fields.isEmpty();
    }

    Shader(String name, Struct instance, Struct fragUniforms, Struct vertUniforms, String vertexFormat, List<Field> instanceVertFields, List<Field> instanceFragFields, List<Field> vertOut) {
        // TODO: could support passing structs instead of a vertexFormat.
        int layout = 0;
        if (vertexFormat.equals("PT")) {
            var field = new Field(new IdentifierDataType("Vec2"), "vertexPosition", Optional.empty());
            vertexAttributes.add(new VertexAttribute(layout, field, false));
            layout++;
            field = new Field(new IdentifierDataType("Vec2"), "vertexTexturePosition", Optional.empty());
            vertexAttributes.add(new VertexAttribute(layout, field, false));
            layout++;
        }
        for (var field : instance.fields) {
            var f = new Field(field.dataType, "instance" + FormatUtils.firstLetterToUppercase(field.name), field.defaultValueCppSource);
            vertexAttributes.add(new VertexAttribute(layout, f, true));
            if (f.dataType.getName().equals("Mat3x2")) {
                layout += 3;
            } else {
                layout++;
            }
        }
        this.name = name;
        this.instance = instance;
        this.fragUniforms = fragUniforms;
        this.vertUniforms = vertUniforms;
        this.vertexFormat = vertexFormat;
        this.instanceVertFields = instanceVertFields;
        this.instanceFragFields = instanceFragFields;
        this.vertOut = vertOut;
    }

    String getNameFirstLetterLowercase() {
        return FormatUtils.firstLetterToLowercase(name);
    }
}

class VertexAttribute {
    public int layout;
    public Field field;
    public boolean isPerInstance;

    VertexAttribute(int layout, Field field, boolean isPerInstance) {
        this.layout = layout;
        this.field = field;
        this.isPerInstance = isPerInstance;
    }

    public String getNameWithoutPrefix() {
        var noInstance = field.name.replace("instance", "");
        return FormatUtils.firstLetterToLowercase(noInstance);
    }
}

abstract class EnumAttribute { }

class EnumAttributeToStr extends EnumAttribute { }
class EnumAttributeImGuiCombo extends EnumAttribute { }

class Cpp extends Declaration {
    public String cppSource;

    Cpp(String source) {
        this.cppSource = source;
    }
}

// Maybe allow normal declarations inside structs This would allows things like creating nested enums. Is this worth it? Would also require having to make the enums outside the class which shouldn't be that much of an issue.
// Namespaces are also not supported.
class DeclarationInStruct {
    public boolean getIsField() {
        return this instanceof Field;
    }

    public boolean getIsCppInStruct() {
        return this instanceof CppInStruct;
    }
}

class Field extends DeclarationInStruct {
    public DataType dataType;
    public String name;
    public Optional<String> defaultValueCppSource;

    Field(DataType dataType, String name, Optional<String> defaultValueCppSource) {
        this.dataType = dataType;
        this.name = name;
        this.defaultValueCppSource = defaultValueCppSource;
    }

    public String getDisplayName() {
        return FormatUtils.camelCaseToFirstLetterUppercaseWithSpaces(name);
    }
    public String getNameFirstLetterToUppercase() {
        return FormatUtils.firstLetterToUppercase(name);
    }

    public boolean getHasDefaultValue() {
        return defaultValueCppSource.isPresent();
    }
    public String getDefaultValue() {
        return defaultValueCppSource.get();
    }

}

class CppInStruct extends DeclarationInStruct {
    String cppSource;

    CppInStruct(String source) {
        this.cppSource = source;
    }
}

abstract class FieldProperty { }

abstract class DataType {
    abstract public String getName();
    public String getNameUpperSnakeCase() {
        return FormatUtils.camelCaseToUpperSnakeCase(getName());
    }
    public String getNameFirstLetterLowercase() {
        return FormatUtils.firstLetterToLowercase(getName());
    }

    public boolean getIsFloat() {
        return getName().equals("float");
    }
    public boolean getIsRangedFloat() {
        return this instanceof RangedFloatDataType;
    }
    public boolean getIsI32() {
        return this instanceof I32DataType;
    }
    public boolean getIsRangedSignedInt() {
        return this instanceof RangedSignedIntDataType;
    }
    public boolean getIsBool() {
        return this instanceof BoolDataType;
    }
    public boolean getIsColor() {
        return this instanceof ColorDataType;
    }
    public boolean getIsVec2() {
        return getName().equals("Vec2");
    };
    public boolean getIsVec3() {
        return getName().equals("Vec3");
    };
    public boolean getIsVec4() {
        return getName().equals("Vec4");
    };
    public boolean getIsVector() {
        return this instanceof VectorDataType;
    }
    public boolean getIsIdentifier() {
        return this instanceof IdentifierDataType;
    }
    public boolean getIsMat3x2() {
        return getName().equals("Mat3x2");
    };
}

class FloatDataType extends DataType {
    @Override
    public String getName() {
        return "float";
    }
}

class RangedFloatDataType extends DataType {
    public float min, max;

    RangedFloatDataType(float min, float max) {
        this.min = min;
        this.max = max;
    }

    @Override
    public String getName() {
        return "float";
    }
}

class I32DataType extends DataType {
    @Override
    public String getName() {
        return "i32";
    }
}

class BoolDataType extends DataType {
    @Override
    public String getName() {
        return "bool";
    }
}

class ColorDataType extends DataType {
    @Override
    public String getName() {
        return "Vec4";
    }
}

class RangedSignedIntDataType extends DataType {
    public String min, max;

    RangedSignedIntDataType(String min, String max) {
        this.min = min;
        this.max = max;
    }

    @Override
    public String getName() {
        return "i32";
    }
}

class VectorDataType extends DataType {
    public DataType itemDataType;

    VectorDataType(DataType itemDataType) {
        this.itemDataType = itemDataType;
    }

    @Override
    public String getName() {
        return "std::vector<" + itemDataType.getName() + ">";
    }
}

class IdentifierDataType extends DataType {
    public String typeNameIdentifier;

    IdentifierDataType(String typeNameIdentifier) {
        this.typeNameIdentifier = typeNameIdentifier;
    }

    @Override
    public String getName() {
        return typeNameIdentifier;
    }
}
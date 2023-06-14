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

    public Iterator<Field> getFields() {
        var stream =  declarations
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
}

abstract class StructAttribute { }

class StructAttributeNetworkSerialize extends StructAttribute { }
class StructAttributeGui extends StructAttribute { }
class StructAttributeJson extends StructAttribute { }

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
    public String getDefaultValue() {
        return defaultValueCppSource.get();
    }

    public boolean getHasDefaultValue() {
        return defaultValueCppSource.isPresent();
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

    public boolean getIsFloat() {
        return this instanceof FloatDataType;
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
    public boolean getIsVector() {
        return this instanceof VectorDataType;
    }
    public boolean getIsIdentifier() {
        return this instanceof IdentifierDataType;
    }
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
    public long min, max;

    RangedSignedIntDataType(long min, long max) {
        this.min = min;
        this.max = max;
    }

    @Override
    public String getName() {
        if (this.max <= Byte.MAX_VALUE && this.min >= Byte.MIN_VALUE) {
            return "i8";
        } else if (this.max <= Short.MAX_VALUE && this.min >= Short.MIN_VALUE) {
            return "i16";
        } else if (this.max <= Integer.MAX_VALUE && this.min >= Integer.MIN_VALUE) {
            return "i32";
        } else {
            // serialize_int doesn't support values bigger than 32 bits. TODO: Maybe support this later.
            return "error";
        }
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
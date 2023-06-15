public class Config {
    private static IncludePath MATH_PATH = new IncludePath("engine/Math/");
    public static IncludePath VEC2_PATH = new IncludePath(MATH_PATH.path + "Vec2.hpp");
    public static IncludePath VEC3_PATH = new IncludePath(MATH_PATH.path + "Vec3.hpp");
    public static IncludePath VEC4_PATH = new IncludePath(MATH_PATH.path + "Vec4.hpp");
    public static IncludePath NETWORKING_PATH = new IncludePath("yojimbo/yojimbo.h");
    public static IncludePath GUI_PATH = new IncludePath("Gui.hpp");
    public static IncludePath JSON_PATH = new IncludePath("Json.hpp");
    public static IncludePath TYPES_PATH = new IncludePath("Types.hpp");

    public static String GENERATED_DIRECTORY = "generated";
    public static String EXTENSION = ".data";
}

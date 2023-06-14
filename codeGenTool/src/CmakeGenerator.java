import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;

public class CmakeGenerator {
    public static void main(String[] args) {
        /*var path = "./client";*/
        var path = "client";
        List<String> generatedFiles = new ArrayList<>();
        List<String> dependencies = new ArrayList<>();
        try (Stream<Path> files = Files.walk(Paths.get(path))) {
            var filesIterator = files.iterator();

            while (filesIterator.hasNext()) {
                var file = filesIterator.next();
                if (!Main.isDataFile(file)) {
                    continue;
                }
                var paths = new GeneratedFilesPaths(file.toString());
                generatedFiles.add(paths.cppFileName.replace('\\', '/'));
                dependencies.add(paths.dataFile.replace('\\', '/'));
            }
        } catch (java.io.IOException v) {
            System.err.println("path doesn't exist");
            return;
        }
        var output = "set(GENERATED_FILES\n";
        for (var file : generatedFiles) {
            output += "\t${GENERATED_PATH}/" + file + "\n";
        }
        output += ")\n\n";

        output += "set(DEPENDENCIES_FILES\n";
        for (var file : dependencies) {
            output += "\t" + file + "\n";
        }
        output += ")";
        System.out.println(output);
        Main.writeStringToFile("generated/cmake.txt", output);
    }
}

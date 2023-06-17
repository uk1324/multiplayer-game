import org.stringtemplate.v4.*;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Optional;
import java.util.stream.Stream;

class GeneratedFilesPaths {
    public String dataFile;

    public String hppFileName;
    public String hppFilePath;
    public String cppFileName;
    public String cppFilePath;

    public String directory;
    public String fileName;

    public String hppFilePathRelativeToCppFile;

    GeneratedFilesPaths(String file) {
        this.dataFile = file;
        var slashIndex = file.lastIndexOf('/');
        var backSlashIndex = file.lastIndexOf('\\');
        // Handles -1 correctly.
        var fileNameStart = slashIndex > backSlashIndex ? slashIndex : backSlashIndex + 1;
        this.fileName = file.substring(fileNameStart, file.length() - Config.EXTENSION.length());
        this.directory = file.substring(0, fileNameStart);

        this.hppFileName = fileName + "Data.hpp";
        this.cppFileName = fileName + "DataGenerated.cpp";

        this.hppFilePath = Paths.get(this.directory, this.hppFileName).toString();
        this.cppFilePath = Paths.get(Config.GENERATED_DIRECTORY, cppFileName).toString();

        this.hppFilePathRelativeToCppFile = Paths.get(Config.GENERATED_DIRECTORY).relativize(Paths.get(this.hppFilePath)).toString();
    }
}

public class Main {
    public static void main(String[] args) {
        if (args.length == 2) {
            System.out.println(args[0]);
            System.out.println(args[1]);
            Config.GENERATED_DIRECTORY = args[1];
            processPath(Path.of(args[0]));
        } else if (args.length == 1) {
            var path = args[0];
            try (Stream<Path> files = Files.walk(Paths.get(path))) {
                files.forEach(Main::processPath);
            } catch (java.io.IOException v) {
                System.err.println("path doesn't exist");
            }
        } else {
            System.err.println("wrong number of arguments");
        }
    }

    public static boolean isDataFile(Path file) {
        if (!Files.isRegularFile(file)) {
            return false;
        }
        // Path::endsWith doesn't work.
        return file.toString().endsWith(Config.EXTENSION);
    }

    static void mergeGenerated(String filePath, String newGenerated) {
        var optSource = tryReadFileToString(filePath.toString());
        if (optSource.isEmpty()) {
            return;
        }
        var source = optSource.get();
        var GENERATED_END = "/*generated end*/";
        var split = source.indexOf(GENERATED_END);
        if (split == -1) {
            System.err.format("'%s' contains no '%s'", filePath, GENERATED_END);
            return;
        }
        source = source.substring(split + GENERATED_END.length(), source.length());
        writeStringToFile(filePath, newGenerated + source);
    }

    static void processPath(Path path) {
        if (!isDataFile(path)) {
            return;
        }
        var file = path.toString();

        System.out.format("processing %s\n", file);
        var optDataFile = readAndParseDataFile(file);
        if (optDataFile.isEmpty()) {
            return;
        }
        var dataFile = optDataFile.get();
        var paths = new GeneratedFilesPaths(file);

        {
            var group = new STGroupFile("dataFileHpp.stg");
            ST st = group.getInstanceOf("dataFile");
            st.add("dataFile", dataFile);
            System.out.format("generating %s\n", paths.hppFilePath);
            writeStringToFile(paths.hppFilePath, st.render());
        }

        {
            var group = new STGroupFile("dataFileCpp.stg");
            ST st = group.getInstanceOf("dataFile");
            st.add("dataFile", dataFile);
            st.add("hppPath", paths.hppFilePathRelativeToCppFile);
            try {
                Files.createDirectories(Paths.get(Config.GENERATED_DIRECTORY));
            } catch (IOException e) {
                System.err.format("failed to create directory '%s'\n", Config.GENERATED_DIRECTORY);
            }
            System.out.format("generating %s\n", paths.cppFilePath);
            writeStringToFile(paths.cppFilePath, st.render());
        }

        for (var declaration : dataFile.declarations) {
            if (!(declaration instanceof Shader)) {
                continue;
            }
            var shader = (Shader)declaration;
            var group = new STGroupFile("shader.stg");

            {
                ST st = group.getInstanceOf("vert");
                st.add("shader", shader);
                var vertFilePath = Paths.get(paths.directory, paths.fileName + ".vert");
                var vertFile = new File(vertFilePath.toString());
                if (vertFile.exists() && vertFile.isFile()) {
                    st.add("generatedForFirstTime", false);
                    mergeGenerated(vertFilePath.toString(), st.render());
                } else {
                    st.add("generatedForFirstTime", true);
                    writeStringToFile(vertFilePath.toString(), st.render());
                }
            }

            {
                ST st = group.getInstanceOf("frag");
                st.add("shader", shader);
                var fragFilePath = Paths.get(paths.directory, paths.fileName + ".frag");
                var vertFile = new File(fragFilePath.toString());
                if (vertFile.exists() && vertFile.isFile()) {
                    st.add("generatedForFirstTime", false);
                    mergeGenerated(fragFilePath.toString(), st.render());
                } else {
                    st.add("generatedForFirstTime", true);
                    writeStringToFile(fragFilePath.toString(), st.render());
                }
            }
        }
    }

    static Optional<String> tryReadFileToString(String path) {
        try {
            return Optional.of(Files.readString(Path.of(path)));
        } catch(IOException e) {
            System.err.format("failed to read %s", path);
            return Optional.empty();
        }
    }

    static Optional<DataFile> readAndParseDataFile(String file) {
        var optSource = tryReadFileToString(file);
        if (optSource.isEmpty()) {
            return Optional.empty();
        }
        String source = optSource.get();

        var parser = new Parser(source);
        DataFile dataFile;
        try {
            dataFile = parser.parse();
        } catch (LexerError e) {
            System.err.println("failed to lex file");
            System.err.println(e.getMessage());
            return Optional.empty();
        } catch (ParserError e) {
            System.err.println("failed to parse file");
            System.err.println(e.getMessage());
            return Optional.empty();
        }

        return Optional.of(dataFile);
    }

    static void writeStringToFile(String path, String data) {
        try (PrintWriter headerFile = new PrintWriter(path)) {
            headerFile.print(data);
        } catch (IOException e) {
            System.err.format("failed to write to file %s\n", path);
        }
    }

}
import org.stringtemplate.v4.*;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.file.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.stream.Stream;

// TODO: rework how paths work, what are the inputs to the program (absolute / relative). Where is the working directory of the executable. etc. Slashes have to be escaped correctly.

class GeneratedFilesPaths {
    public String absoluteFilePath;
    public String cppExecutableWorkingDirectory;
    public String fileDirectory;
    public String hppFilePath;
    public String cppFilePath;
    public String hppFilePathRelativeToCppFile;

    GeneratedFilesPaths(String file, String generatedOutDirectory, String cppExecutableWorkingDirectory) {
        generatedOutDirectory = Paths.get(generatedOutDirectory).toAbsolutePath().toString();
        this.cppExecutableWorkingDirectory = Paths.get(cppExecutableWorkingDirectory).toAbsolutePath().toString();
        var absolutePath = Paths.get(file).toAbsolutePath();
        this.absoluteFilePath = absolutePath.toString();

        var fileNameWithExtension = absolutePath.getFileName().toString();
        var extensionStart = fileNameWithExtension.lastIndexOf(".");
        // When equal to 0 then leave it for example .gitignore would remain the same.
        var fileNameWithoutExtension = extensionStart > 0 ? fileNameWithExtension.substring(0, extensionStart) : fileNameWithExtension;

        var fileDirectoryPathObject = absolutePath.getParent();
        if (fileDirectoryPathObject == null) {
            throw new RuntimeException("absolute path has no directory");
        }
        this.fileDirectory = fileDirectoryPathObject.toString();

        this.hppFilePath = Paths.get(fileDirectory, fileNameWithoutExtension + "Data.hpp").normalize().toString();
        this.cppFilePath = Paths.get(generatedOutDirectory, fileNameWithoutExtension + "DataGenerated.cpp").normalize().toString();
        this.hppFilePathRelativeToCppFile = Paths.get(generatedOutDirectory).relativize(Paths.get(this.hppFilePath)).toString();
    }

    public String getVertPath(String shaderName) {
        return Paths.get(fileDirectory, FormatUtils.firstLetterToLowercase(shaderName) + ".vert").normalize().toString();
    }

    public String getFragPath(String shaderName) {
        return Paths.get(fileDirectory, FormatUtils.firstLetterToLowercase(shaderName) + ".frag").normalize().toString();
    }
}

public class Main {
    static String thisProgramWorkingDirectory = System.getProperty("user.dir");

    public static void main(String[] args) {
        if (args.length >= 1 && args[0].equals("watch")) {
            watchProgram(args);
        } else {
            compilerProgram(args);
        }
    }

    static void compilerProgram(String[] args) {
        if (args.length < 3) {
            System.err.println("wrong number of arguments");
            System.out.println("usage: <cppExecutableWorkingDirectory> <generatedOutDirectory> <folderToProcessRecursivelyOrFile...>");
            return;
        }

        var cppExecutableWorkingDirectory = args[0];
        var generatedOutDirectory = args[1];

        for (int i = 2; i < args.length; i++) {
            var path = args[i];
            var file = new File(path);
            if (file.isFile()) {
                processDataFile(path, generatedOutDirectory, cppExecutableWorkingDirectory);
            } else if (file.isDirectory()) {
                try (Stream<Path> files = Files.walk(Paths.get(path))) {
                    files.forEach((Path filePath) -> {
                        if (isDataFile(filePath)) {
                            processDataFile(filePath.toString(), generatedOutDirectory, cppExecutableWorkingDirectory);
                        }
                    });
                } catch (java.io.IOException v) {
                    System.err.println("path doesn't exist");
                }
            } else {
                System.err.format("invalid path %s", path);
            }
        }
    }

    static void watchProgram(String[] args) {
        if (args.length < 4) {
            System.err.println("wrong number of arguments");
            System.out.println("usage: watch <cppExecutableWorkingDirectory> <generatedOutDirectory> <directoryToRecursivelyToWatch...>");
        }
        var cppExecutableWorkingDirectory = args[1];
        var generatedOutDirectory = args[2];

        List<Path> directoryPaths = new ArrayList<>();
        for (int i = 3; i < args.length; i++) {
            directoryPaths.add(Paths.get(args[i]));
        }
        try {
            new Watcher(directoryPaths, cppExecutableWorkingDirectory, generatedOutDirectory).processEvents();
        } catch (IOException e) {
            System.err.format("watcher error: %s\n", e.getMessage());
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
        var optSource = tryReadFileToString(filePath);
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
        source = source.substring(split + GENERATED_END.length());
        writeStringToFile(filePath, newGenerated + source);
    }

    static void createIfNotExistsElseMerge(ST st, String path) {
        var file = new File(path);
        if (file.exists() && file.isFile()) {
            st.add("generatedForFirstTime", false);
            mergeGenerated(path, st.render());
        } else {
            st.add("generatedForFirstTime", true);
            writeStringToFile(path, st.render());
        }
    }

    // Just for readability of the outputted generated paths.
    static String relativeToThisProgramWorkingDirectory(String path) {
        return Paths.get(thisProgramWorkingDirectory).relativize(Paths.get(path)).toString();
    }

    static void processDataFile(String inputPath, String generatedOutDirectory, String cppExecutableWorkingDirectory) {
        var paths = new GeneratedFilesPaths(inputPath, generatedOutDirectory, cppExecutableWorkingDirectory);
        System.out.format("processing %s\n", inputPath);
        var optDataFile = readAndParseDataFile(paths.absoluteFilePath, paths);
        if (optDataFile.isEmpty()) {
            return;
        }
        var dataFile = optDataFile.get();

        {
            var group = new STGroupFile("dataFileHpp.stg");
            ST st = group.getInstanceOf("dataFile");
            st.add("dataFile", dataFile);
            System.out.format("generating %s\n", relativeToThisProgramWorkingDirectory(paths.hppFilePath));
            writeStringToFile(paths.hppFilePath, st.render());
        }

        {
            var group = new STGroupFile("dataFileCpp.stg");
            ST st = group.getInstanceOf("dataFile");
            st.add("dataFile", dataFile);
            st.add("hppPath", paths.hppFilePathRelativeToCppFile);
            try {
                Files.createDirectories(Paths.get(generatedOutDirectory));
            } catch (IOException e) {
                System.err.format("failed to create directory '%s'\n", generatedOutDirectory);
            }
            System.out.format("generating %s\n", relativeToThisProgramWorkingDirectory(paths.cppFilePath));
            writeStringToFile(paths.cppFilePath, st.render());
        }

        for (var declaration : dataFile.declarations) {
            if (!(declaration instanceof Shader)) {
                continue;
            }
            var shader = (Shader)declaration;
            var group = new STGroupFile("shader.stg");

            if (shader.generateVert) {
                ST st = group.getInstanceOf("vert");
                st.add("shader", shader);
                System.out.format("generating %s\n", relativeToThisProgramWorkingDirectory(shader.vertPath));
                createIfNotExistsElseMerge(st, shader.vertPath);
            }

            {
                ST st = group.getInstanceOf("frag");
                st.add("shader", shader);
                System.out.format("generating %s\n", relativeToThisProgramWorkingDirectory(shader.fragPath));
                createIfNotExistsElseMerge(st, shader.fragPath);
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

    static Optional<DataFile> readAndParseDataFile(String file, GeneratedFilesPaths paths) {
        var optSource = tryReadFileToString(file);
        if (optSource.isEmpty()) {
            return Optional.empty();
        }
        String source = optSource.get();

        var parser = new Parser(source, paths);
        DataFile dataFile;
        try {
            dataFile = parser.parse();
        } catch (LexerError e) {
            System.err.println("failed to lex file");
            System.err.println(e.getMessage());
            return Optional.empty();
        } catch (ParserError e) {
            System.err.println("failed to parse file");
            if (e.line.isPresent()) {
                System.err.format("error on line %s\n", e.line.get());
            }
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
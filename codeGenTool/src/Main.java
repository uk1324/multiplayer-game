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

        this.hppFilePathRelativeToCppFile = Paths.get("..", this.directory, this.hppFileName).toString();
    }
}

public class Main {
    /*public static void*/

    public static void main(String[] args) {
        var path = "./client";
        try (Stream<Path> files = Files.walk(Paths.get(path))) {
            files.forEach(Main::processPath);
        } catch (java.io.IOException v) {
            System.err.println("path doesn't exist");
        }
    }

    public static boolean isDataFile(Path file) {
        if (!Files.isRegularFile(file)) {
            return false;
        }
        // Path::endsWith doesn't work.
        return file.toString().endsWith(Config.EXTENSION);
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
            writeStringToFile(paths.hppFilePath, st.render());
        }

        {
            var group = new STGroupFile("dataFileCpp.stg");
            ST st = group.getInstanceOf("dataFile");
            st.add("dataFile", dataFile);
            st.add("hppPath", paths.hppFilePathRelativeToCppFile);
            //var cppPath = fileDirectory + fileName + ".cpp";
            try {
                Files.createDirectories(Paths.get(Config.GENERATED_DIRECTORY));
            } catch (IOException e) {
                System.err.format("failed to create directory '%s'\n", Config.GENERATED_DIRECTORY);
            }
            writeStringToFile(paths.cppFilePath, st.render());
        }
    }

    static Optional<DataFile> readAndParseDataFile(String file) {
        String source;
        try {
            source = Files.readString(Path.of(file));
        } catch(IOException e) {
            System.err.println("failed to read file");
            return Optional.empty();
        }

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
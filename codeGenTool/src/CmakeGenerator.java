import com.sun.nio.file.ExtendedWatchEventModifier;

import java.nio.file.*;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;

public class CmakeGenerator {

    static String path;

    public static void main(String[] args) {
        path = "./client";
        updateCmakeGeneratedFilesList();
        watchDirectory(path);
    }

    // https://fullstackdeveloper.guru/2020/12/23/how-to-watch-a-folder-directory-or-changes-using-java/
    public static void watchDirectory(String path) {
        try {
            System.out.format("Watching %s for changes\n", path);
            WatchService watchService = FileSystems.getDefault().newWatchService();

            var directory = Path.of(path);

            WatchEvent.Kind<Path>[] events = new WatchEvent.Kind[2];
            events[0] = StandardWatchEventKinds.ENTRY_CREATE;
            events[1] = StandardWatchEventKinds.ENTRY_DELETE;
            WatchKey watchKey = directory.register(watchService, events, ExtendedWatchEventModifier.FILE_TREE);

            for (;;) {
                for (WatchEvent<?> event : watchKey.pollEvents()) {

                    var pathEvent = (WatchEvent<Path>) event;

                    Path fileName = pathEvent.context();

                    if (!fileName.toString().endsWith(Config.EXTENSION)) {
                        continue;
                    }

                    WatchEvent.Kind<?> kind = event.kind();
                    if (kind == StandardWatchEventKinds.ENTRY_CREATE) {
                        System.out.println("File created : " + fileName);
                        updateCmakeGeneratedFilesList();
                    } else if (kind == StandardWatchEventKinds.ENTRY_DELETE) {
                        System.out.println("File deleted: " + fileName);
                        updateCmakeGeneratedFilesList();
                    }
                }

                boolean valid = watchKey.reset();
                if (!valid) {
                    break;
                }
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    static void updateCmakeGeneratedFilesList() {
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

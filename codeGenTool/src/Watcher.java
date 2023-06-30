import java.awt.*;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.file.*;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static java.nio.file.LinkOption.NOFOLLOW_LINKS;
import static java.nio.file.StandardWatchEventKinds.*;

// https://docs.oracle.com/javase/tutorial/essential/io/
// https://docs.oracle.com/javase/tutorial/essential/io/examples/WatchDir.java
public class Watcher {
    private final WatchService watcher;
    private final Map<WatchKey, Path> keys;

    private final String cppExecutableWorkingDirectory;
    private final String generatedOutDirectory;

    Watcher(List<Path> directories, String cppExecutableWorkingDirectory, String generatedOutDirectory) throws IOException {
        this.watcher = FileSystems.getDefault().newWatchService();
        this.keys = new HashMap<>();
        for (var directory : directories) {
            registerAll(directory);
        }
        this.cppExecutableWorkingDirectory = cppExecutableWorkingDirectory;
        this.generatedOutDirectory = generatedOutDirectory;
    }

    @SuppressWarnings("unchecked")
    static <T> WatchEvent<T> cast(WatchEvent<?> event) {
        return (WatchEvent<T>)event;
    }

    private void register(Path dir) throws IOException {
        WatchKey key = dir.register(watcher, ENTRY_CREATE, ENTRY_DELETE, ENTRY_MODIFY);
        Path prev = keys.get(key);
        if (prev == null) {
            System.out.format("register: %s\n", dir);
        } else {
            if (!dir.equals(prev)) {
                System.out.format("update: %s -> %s\n", prev, dir);
            }
        }
        keys.put(key, dir);
    }

    private void registerAll(final Path start) throws IOException {
        Files.walkFileTree(start, new SimpleFileVisitor<>() {
            @Override
            public FileVisitResult preVisitDirectory(Path dir, BasicFileAttributes attrs) throws IOException {
                register(dir);
                return FileVisitResult.CONTINUE;
            }
        });
    }

    void processEvents() {
        for (;;) {

            // wait for key to be signalled
            WatchKey key;
            try {
                key = watcher.take();
            } catch (InterruptedException x) {
                return;
            }

            Path dir = keys.get(key);
            if (dir == null) {
                System.err.println("WatchKey not recognized!!");
                continue;
            }

            for (WatchEvent<?> event: key.pollEvents()) {
                WatchEvent.Kind<?> kind = event.kind();

                // TBD - provide example of how OVERFLOW event is handled
                if (kind == OVERFLOW) {
                    continue;
                }

                // Context for directory entry event is the file name of entry
                WatchEvent<Path> ev = cast(event);
                Path name = ev.context();
                Path path = dir.resolve(name);

                if (kind == ENTRY_CREATE) {
                    try {
                        if (Files.isDirectory(path, NOFOLLOW_LINKS)) {
                            System.out.format("directory created: %s\n", path);
                            registerAll(path);
                        }
                    } catch (IOException x) {
                        // ignore to keep sample readable
                    }
                } else if (kind == ENTRY_MODIFY && Main.isDataFile(path)) {
                    System.out.format("file modified: %s\n", path);

                    // Create a stream to hold the output
                    var byteArrayOutputStream = new ByteArrayOutputStream();
                    var printStream = new PrintStream(byteArrayOutputStream);
                    PrintStream old = System.err;
                    System.setErr(printStream);

                    Main.processDataFile(path.toString(), generatedOutDirectory, cppExecutableWorkingDirectory);

                    System.err.flush();
                    System.setErr(old);
                    String errorOutput = byteArrayOutputStream.toString();
                    System.err.print(errorOutput);
                    if (!errorOutput.isEmpty()) {
                        Toolkit.getDefaultToolkit().beep();
                    }
                }
            }

            // reset key and remove from set if directory no longer accessible
            boolean valid = key.reset();
            if (!valid) {
                keys.remove(key);

                // all directories are inaccessible
                if (keys.isEmpty()) {
                    break;
                }
            }
        }
    }

}

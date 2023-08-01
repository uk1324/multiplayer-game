#include <iostream>
#include <filesystem>

using namespace std;
using namespace filesystem;

const path outputPath = "./generated/build/";

void outputClient(const path& exePath) {
	const auto clientOutputPath = outputPath / "client";

	create_directories(clientOutputPath);
	copy_file("./freetype.dll", clientOutputPath / "freetype.dll", copy_options::overwrite_existing);
	copy("./assets", clientOutputPath / "assets", copy_options::recursive | copy_options::overwrite_existing);
	copy_file(exePath / "client/client.exe", clientOutputPath / "game.exe", copy_options::overwrite_existing);
}

void outputServer(const path& exePath) {
	const auto serverOutputPath = outputPath / "server";

	create_directories(serverOutputPath);
	copy_file(exePath / "server/server.exe", serverOutputPath / "server.exe", copy_options::overwrite_existing);
}

int main(int argc, char** argv) {
	cout << "working directory = {}\n" << current_path().string();

	if (argc != 3) {
		cerr << "wrong number of arguments " << argc << '\n';
		return EXIT_FAILURE;
	}
	const string_view typeString(argv[1]);
	const string_view exePathString(argv[2]);

	const path exePath(exePathString);
	try {
		if (typeString == "client") {
			cout << "output client\n";
			outputClient(exePath);
		} else if (typeString == "server") {
			cout << "output server\n";
			outputServer(exePath);
		}
	} catch (const filesystem_error& e) {
		cerr << e.what() << '\n';
		//return EXIT_FAILURE;
	}
}
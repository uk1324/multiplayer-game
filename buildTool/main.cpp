#include <iostream>
#include <filesystem>
#include <format>

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
	copy_file(exePath / "server/serve.exe", serverOutputPath / "server.exe");
}

int main(int argc, char** argv) {
	cout << format("working directory = {}\n", current_path().string());
	static constexpr auto exePath = true ? "./out/build/x64-Debug" : "./out/build/x64-Release";

	if (argc != 2) {
		cerr << "wrong number of arguments" << '\n';
		return EXIT_FAILURE;
	}
	const string_view typeString(argv[1]);

	try {
		if (typeString == "client") {
			outputClient(exePath);
		} else if (typeString == "server") {
			outputServer(exePath);
		}
	} catch (const filesystem_error& e) {
		cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}
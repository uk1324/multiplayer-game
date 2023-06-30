#include <Utils/FileIo.hpp>
#include <Log/Log.hpp>

#include <fstream>

std::string stringFromFile(std::string_view path) {
	auto optString = tryLoadStringFromFile(path);
	if (optString.has_value())
		return std::move(*optString);

	LOG_FATAL("couldn't read file \"%s\"", path.data());
}

std::optional<std::string> tryLoadStringFromFile(std::string_view path) {
	std::ifstream file(path.data(), std::ios::binary);

	if (file.fail())
		return std::nullopt;

	auto start = file.tellg();
	file.seekg(0, std::ios::end);
	auto end = file.tellg();
	file.seekg(start);
	auto fileSize = end - start;

	std::string result;
	// Pointless memset
	result.resize(fileSize);

	file.read(result.data(), fileSize);
	if (file.fail())
		return std::nullopt;

	return result;
}

Json::Value jsonFromFile(std::string_view path) {
	auto optJson = tryLoadJsonFromFile(path);
	if (optJson.has_value()) {
		return std::move(*optJson);
	}
	LOG_FATAL("couldn't parse json file \"%s\"", path.data());
	return Json::Value::null();
}

std::optional<Json::Value> tryLoadJsonFromFile(std::string_view path) {
	try {
		return Json::parse(stringFromFile(path));
	} catch (const Json::ParsingError&) {
		return std::nullopt;
	}
}

ByteBuffer byteBufferFromFile(std::string_view path) {
	std::ifstream file(path.data(), std::ios::binary);

	if (file.fail())
		LOG_FATAL("couldn't open file \"%s\"", path.data());

	auto start = file.tellg();
	file.seekg(0, std::ios::end);
	auto end = file.tellg();
	file.seekg(start);
	auto fileSize = end - start;

	ByteBuffer result(fileSize);

	file.read(result.data(), fileSize);
	if (file.fail())
		LOG_FATAL("couldn't read file \"%s\"", path.data());

	return result;
}

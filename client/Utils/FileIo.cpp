#include <Utils/FileIo.hpp>
#include <Log/Log.hpp>

#include <fstream>

std::string stringFromFile(std::string_view path)
{
	std::ifstream file(path.data(), std::ios::binary);

	if (file.fail())
		LOG_FATAL("couldn't open file \"%s\"", path.data());

	auto start = file.tellg();
	file.seekg(0, std::ios::end);
	auto end = file.tellg();
	file.seekg(start);
	auto fileSize = end - start;

	// Pointless memset
	std::string result;

	result.resize(fileSize);

	file.read(result.data(), fileSize);
	if (file.fail())
		LOG_FATAL("couldn't read file \"%s\"", path.data());

	return result;
}

Json::Value jsonFromFile(std::string_view path)
{
	try
	{
		return Json::parse(stringFromFile(path));
	}
	catch (const Json::ParsingError&)
	{
		LOG_FATAL("couldn't parse json file \"%s\"", path.data());
	}
}

ByteBuffer byteBufferFromFile(std::string_view path)
{
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

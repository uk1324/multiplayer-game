#include "PreprocessIncludes.hpp"
#include "FileIo.hpp"
#include "Overloaded.hpp"
#include <filesystem>

static std::expected<std::string, PreprocessIncludesError> preprocessIncludesImplementation(std::string_view path, int depth) {
	using namespace PreprocessIncludesErrors;

	if (depth > 5) {
		return std::unexpected(MaximumIncludeDepthExceeded{});
	}
	auto source = tryLoadStringFromFile(path);
	if (!source.has_value()) {
		return std::unexpected(FailedToOpen{ .path = std::string(path) });
	}
	std::string_view sourceView = *source;
	auto sourceFolder = std::filesystem::path(path).parent_path();

	size_t offset = 0;
	std::vector<std::string> parts;
	for (;;) {
		std::string_view includeString = "#include \"";
		auto pathStart = sourceView.find(includeString, offset);
		if (pathStart == std::string::npos)
			break;
		pathStart += includeString.size();

		auto pathEnd = sourceView.find('"', pathStart);
		if (pathEnd == std::string::npos)
			break;

		auto pathRelativeToSourceFolder = sourceView.substr(pathStart, pathEnd - pathStart);
		auto pathRelativeToWorkspace = sourceFolder / pathRelativeToSourceFolder;
		auto pathString = pathRelativeToWorkspace.string();

		parts.push_back(std::string(sourceView.substr(offset, pathStart - includeString.size() - offset)));
		auto optSource = preprocessIncludesImplementation(pathString, depth + 1);
		if (!optSource.has_value()) {
			//return std::unexpected(std::move(optSource.error()));
			return optSource;
		}
		parts.push_back(*optSource);

		offset = pathEnd + 1;
	}

	if (parts.empty())
		return std::move(*source);

	parts.push_back(std::string(sourceView.substr(offset)));

	std::string output;
	for (const auto& part : parts)
		output += part;
	return output;
}

std::expected<std::string, PreprocessIncludesError> preprocessIncludes(std::string_view path) {
	return preprocessIncludesImplementation(path, 0);
}

std::ostream& operator<<(std::ostream& os, const PreprocessIncludesError& e) {
	using namespace PreprocessIncludesErrors;

	std::visit(overloaded{
		[&](const MaximumIncludeDepthExceeded&) {
			os << "maximum include depth exceeded";
		},
		[&](const FailedToOpen& e) {
			os << "failed to open '" << e.path << "'";
		}
	}, e);

	return os;
}

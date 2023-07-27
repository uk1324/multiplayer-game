#pragma once

#include <expected>
#include <variant>
#include <string>

namespace PreprocessIncludesErrors {
	struct MaximumIncludeDepthExceeded {};
	struct FailedToOpen {
		std::string path;
	};
};

using PreprocessIncludesError = std::variant<PreprocessIncludesErrors::MaximumIncludeDepthExceeded, PreprocessIncludesErrors::FailedToOpen>;

std::ostream& operator<<(std::ostream& os, const PreprocessIncludesError& e);

std::expected<std::string, PreprocessIncludesError> preprocessIncludes(std::string_view path);
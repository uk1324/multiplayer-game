add_library(
	json
	"JsonValue.cpp"
	"JsonPrinter.cpp"
	"JsonParser.cpp"
)

target_include_directories(json PUBLIC "../Utils")
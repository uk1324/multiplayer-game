#pragma once

namespace Log
{
	void info(const char* filename, int line, const char* format, ...);
	void warning(const char* filename, int line, const char* format, ...);
	void error(const char* filename, int line, const char* format, ...);
	[[noreturn]] void fatal(const char* filename, int line, const char* format, ...);
	void debug(const char* filename, int line, const char* format, ...);
}

#define LOG_INFO(format, ...) \
	Log::info(__FILE__, __LINE__, format, __VA_ARGS__)

#define LOG_WARNING(format, ...) \
	Log::warning(__FILE__, __LINE__, format, __VA_ARGS__)

#define LOG_ERROR(format, ...) \
	Log::error(__FILE__, __LINE__, format, __VA_ARGS__)

#define LOG_FATAL(format, ...) \
	Log::fatal(__FILE__, __LINE__, format, __VA_ARGS__)

#define LOG_DEBUG(format, ...) \
	Log::debug(__FILE__, __LINE__, format, __VA_ARGS__)
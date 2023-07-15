#include "Log.hpp"
#include <Assertions.hpp>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void Log::info(const char* filename, int line, const char* format, ...)
{
	printf("[%s:%d] info: ", filename, line);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void Log::warning(const char* filename, int line, const char* format, ...)
{
	printf("[%s:%d] warning: ", filename, line);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void Log::error(const char* filename, int line, const char* format, ...)
{
	printf("[%s:%d] error: ", filename, line);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void Log::fatal(const char* filename, int line, const char* format, ...)
{
	printf("[%s:%d] fatal: ", filename, line);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
	ASSERT_NOT_REACHED();
	exit(EXIT_FAILURE);
}

void Log::debug(const char* filename, int line, const char* format, ...)
{
	printf("[%s:%d] debug: ", filename, line);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

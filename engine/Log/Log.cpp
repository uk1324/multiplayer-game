#include "Log.hpp"
#include <Assertions.hpp>
#include <CrashReport.hpp>
#include <DebugBreak.hpp>

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

void Log::fatal(const char* filename, int line, const char* functionName, const char* format, ...)
{
	printf("[%s:%d] fatal: ", filename, line);
	va_list args;
	char message[1024];
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	printf("%s\n", message);
	#ifndef FINAL_RELEASE
	DEBUG_BREAK();
	#endif
	crashReportMessageBox(message, functionName, line);
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

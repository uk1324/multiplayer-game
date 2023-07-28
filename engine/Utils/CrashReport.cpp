#include "CrashReport.hpp"
#include <stdio.h>
#include <Windows.h>

void crashReportMessageBox(const char* text, const char* functionName, int line) {
	char message[1024];
	snprintf(message, sizeof(message), "location\n line = %d\nfunction = %s\nmessage\n%s", line, functionName, text);
	MessageBoxA(nullptr, message, "crash report", MB_OK | MB_ICONEXCLAMATION);
	exit(EXIT_FAILURE);
}
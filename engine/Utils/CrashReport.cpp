#include "CrashReport.hpp"
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#elif linux

#endif



void crashReportMessageBox(const char* text, const char* functionName, int line) {
	char message[1024];
	snprintf(message, sizeof(message), "location\n line = %d\nfunction = %s\nmessage\n%s", line, functionName, text);
	#ifdef _WIN32
	MessageBoxA(nullptr, message, "crash report", MB_OK | MB_ICONEXCLAMATION);
	#endif // For other platforms maybe use GTK or SDL. 

	exit(EXIT_FAILURE);
}

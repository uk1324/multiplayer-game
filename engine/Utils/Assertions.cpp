#include "Assertions.hpp"
#include "CrashReport.hpp"
#include <stdio.h>
#include <string.h>

void assertImplementation(bool condition, const char* functionName, int line) {
	if (condition) {
		return;
	}
	#ifndef FINAL_RELEASE
	__debugbreak();
	#endif
	crashReportMessageBox("assertion failed", functionName, line);
}
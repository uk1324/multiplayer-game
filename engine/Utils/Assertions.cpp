#include "Assertions.hpp"
#include "CrashReport.hpp"
#include "DebugBreak.hpp"
#include <stdio.h>
#include <string.h>

void assertImplementation(bool condition, const char* functionName, int line) {
	if (condition) {
		return;
	}
	#ifndef FINAL_RELEASE
	DEBUG_BREAK
	#endif
	crashReportMessageBox("assertion failed", functionName, line);
}
#include "WindowsUtils.hpp"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#include <cstdlib>
#endif

void setConsolePosAndSize(int x, int y, int width, int height) {
	#ifdef _WIN32
	const auto console = GetConsoleWindow();
	if (console == nullptr) {
		return;
	}

	const auto style = GetWindowLongA(console, GWL_STYLE);
	const auto hasMenu = GetMenu(console) != nullptr;
	RECT rect{ .left = x, .top = y, .right = x + width, .bottom = y + height };
	if (!AdjustWindowRect(&rect, style, hasMenu)) {
		return;
	}

	SetWindowPos(console, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
	#endif
}

#ifdef _WIN32
void (*onCloseCallback)() = nullptr;

BOOL WINAPI ctrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
    case CTRL_CLOSE_EVENT:
        onCloseCallback();
        return TRUE;

    default:
        return FALSE;
    }
	
}
#endif

void setOnCloseCallback(void (*function)()) {
	#ifdef _WIN32
    onCloseCallback = function;
    SetConsoleCtrlHandler(ctrlHandler, true);
	atexit(function);
	#endif
}

void getPrimaryScreenSize(int* x, int* y) {
	#ifdef _WIN32
	*x = GetSystemMetrics(SM_CXSCREEN);
	*y = GetSystemMetrics(SM_CYSCREEN);
	#endif
}

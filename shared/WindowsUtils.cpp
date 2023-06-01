#include "WindowsUtils.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void setConsolePosAndSize(int x, int y, int width, int height) {
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
}

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

void setOnCloseCallback(void (*function)()) {
    onCloseCallback = function;
    SetConsoleCtrlHandler(ctrlHandler, true);
}

void getPrimaryScreenSize(int* x, int* y) {
	*x = GetSystemMetrics(SM_CXSCREEN);
	*y = GetSystemMetrics(SM_CYSCREEN);
}

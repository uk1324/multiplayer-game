// This needs to be in it's own file, because raygui has name collisions with winuser.h
#include "init.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_opengl3.h>
#include <windows.h>
#include <Commctrl.h>

#pragma comment(lib, "Comctl32.lib")

#define CHECK_BOOL(expr) \
	do { \
		if (!(expr)) { \
			DebugBreak(); \
		} \
	} while (false)

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	if (const auto result = ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
		return !result;
	}
	switch (uMsg) {
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, SubclassWindowProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// To get input on Windows ImGui wndProcHandler needs to be called. One to do this option would be create a new function in the raylib.lib file and header file which returns the glfw window handle and use the glfw backend. This would also require having the glfw including the glfw header files and there are quite a few of those that imgui needs. Also not sure if it would even link properly. Don't know if the glfw functions are accessible from the raylib.lib file.
// I decided to just add a new window proc function that calls both imgui and the old proc function.
auto initImGui(void* hwnd) -> void {
	ImGui::CreateContext();
	CHECK_BOOL(ImGui_ImplWin32_Init(hwnd));
	CHECK_BOOL(ImGui_ImplOpenGL3_Init("#version 330"));
	// Don't know why, but using SetWindowLongPtr doesn't work so istead SetWindowSubclass is used, which is apparently better(https://devblogs.microsoft.com/oldnewthing/20031111-00/?p=41883), but it requires linking with Comctl32. When using SetWindowLongPtr when glfw calls PeekMessage there is a access violation executing invalid location error. Tried both with and without CallWindowProc.
	SetWindowSubclass(reinterpret_cast<HWND>(hwnd), SubclassWindowProc, 1, 0);

	auto& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("assets/fonts/RobotoMono-Regular.ttf", 20);
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	auto& style = ImGui::GetStyle();
	style.WindowRounding = 5.0f;
}
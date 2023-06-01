#pragma once

#include <Windows.h>
#include <iostream>

bool ctrlHeld = false;
bool shiftHeld = false;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    bool eatKeystroke = false;
    if (nCode == HC_ACTION) {
        switch (wParam) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
            if (p->flags & LLKHF_INJECTED)
                break;
            const auto keycode = p->vkCode;
            
            // https://stackoverflow.com/questions/41200085/how-to-correctly-use-keyboard-hooks-to-press-a-key-combination-to-block-a-key-an
            if (keycode == VK_LCONTROL) {
                ctrlHeld = true;
            } else if (keycode == VK_LSHIFT) {
                shiftHeld = true;
            } else if (ctrlHeld && shiftHeld && keycode == 'Z') {
                std::cout << system("runServerAnd2Clients.bat");
            } else if (ctrlHeld && shiftHeld && keycode == 'X') {
                std::cout << system("taskkill /IM server.exe");
            }

            break;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
            if (p->flags & LLKHF_INJECTED)
                break;
            const auto keycode = p->vkCode;

            if (keycode == VK_LCONTROL) ctrlHeld = false;
            else if (keycode == VK_LSHIFT) shiftHeld = false;
            break;
        }

        }
    }
    return eatKeystroke ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    HHOOK hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);

    MSG msg;
    while (!GetMessage(&msg, NULL, NULL, NULL)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hhkLowLevelKybd);
}


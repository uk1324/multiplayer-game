#pragma once

#include <Windows.h>
#include <iostream>

bool ctrlHeld = false;
bool shiftHeld = false;

enum class Mode {
    SERVER_AND_2_CLIENTS,
    SERVER_AND_CLIENT,
    SERVER,
    CLIENT,
};

Mode mode = Mode::SERVER_AND_2_CLIENTS;

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
                switch (mode) {
                case Mode::SERVER_AND_2_CLIENTS:
                    std::cout << system("runServerAnd2Clients.bat") << '\n';
                    break;
                case Mode::SERVER_AND_CLIENT:
                    std::cout << system("runClientServer.bat") << '\n';
                    break;
                case Mode::SERVER:
                    std::cout << system("runServer.bat") << '\n';
                    break;
                case Mode::CLIENT:
                    std::cout << system("runClient.bat") << '\n';
                    break;

                default:
                    break;
                }
            } else if (ctrlHeld && shiftHeld && keycode == 'X') {
                std::cout << system("taskkill /IM server.exe");
            } else if (keycode == VK_F1) {
                std::cout << "switched mode to server and 2 clients\n";
                mode = Mode::SERVER_AND_2_CLIENTS;
            } else if (keycode == VK_F2) {
                std::cout << "switched mode to server and client\n";
                mode = Mode::SERVER_AND_CLIENT;
            } else if (keycode == VK_F3) {
                std::cout << "switched mode to server\n";
                mode = Mode::SERVER;
            } else if (keycode == VK_F4) {
                std::cout << "switched mode to client\n";
                mode = Mode::CLIENT;
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


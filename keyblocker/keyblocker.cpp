#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#pragma comment(lib, "user32.lib")

HHOOK g_hook = NULL;
bool g_running = true;
bool g_flashing = false;

struct KeyCode {
    int code;
    std::string name;
};

const std::vector<KeyCode> keyCodes = {
    {VK_CONTROL, "CTRL"},
    {'P', "P"},
};

bool parseKeyCombination(const std::string& combo, int& modifiers, int& key) {
    modifiers = 0;
    key = 0;
    
    std::string upperCombo = combo;
    for (char& c : upperCombo) {
        c = std::toupper(c);
    }
    
    if (upperCombo.find("CTRL") != std::string::npos) {
        modifiers |= MOD_CONTROL;
    }
    
    for (const auto& kc : keyCodes) {
        if (upperCombo.find(kc.name) != std::string::npos && kc.code != VK_CONTROL && kc.code != VK_SHIFT && kc.code != VK_MENU && kc.code != VK_ESCAPE) {
            key = kc.code;
            break;
        }
    }
    
    return key != 0;
}

std::string getKeyName(int code) {
    for (const auto& kc : keyCodes) {
        if (kc.code == code) {
            return kc.name;
        }
    }
    return "UNKNOWN";
}

std::string getModifierNames(int modifiers) {
    std::string result;
    if (modifiers & MOD_CONTROL) result += "CTRL+";
    return result;
}

HWND createRedOverlay() {
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASS wc = {};
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"KeyBlockerOverlay";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClass(&wc);
        classRegistered = true;
    }
    
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"KeyBlockerOverlay",
        L"CTRL + P",
        WS_POPUP,
        10, 10, 200, 50,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    if (hwnd) {
        HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)redBrush);
        
        SetLayeredWindowAttributes(hwnd, 0, 200, LWA_ALPHA);
        
        HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
    
    return hwnd;
}

void showRedTextOverlay() {
    if (g_flashing) return;
    g_flashing = true;
    
    std::thread overlayThread([]() {
        HWND overlay = createRedOverlay();
        if (overlay) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            DestroyWindow(overlay);
        }
        
        g_flashing = false;
    });
    
    overlayThread.detach();
}


LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            int vkCode = pKeyboard->vkCode;
            
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                if (ctrlPressed && (vkCode == 'P' || vkCode == 'p')) {
                    showRedTextOverlay();
                    return 1;
                }
            }
            
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                bool pPressed = (GetKeyState('P') & 0x8000) != 0;
                if (pPressed && vkCode == VK_CONTROL) {
                    return 1;
                }
            }
        }
    }
    
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

void showHelp() {
    std::cout << "\n=== keyblocker ===" << std::endl;
    std::cout << "blocks ctrl + p when running" << std::endl;
}

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT) {
        g_running = false;
        std::cout << "\nexiting keyblocker..." << std::endl;
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char* argv[]) {

    std::cout << "created for screensharing on rustdesk privacy mode by blocking user input" << std::endl;

    std::cout << "https://github.com/whyamidoingthisxd" << std::endl;

    std::cout << "\nkeyblocker - starting..." << std::endl;
    
    showHelp();
    
    std::cout << "\nstatus: ctrl + p blocking active" << std::endl;
    
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    
    g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    
    if (g_hook == NULL) {
        DWORD error = GetLastError();
        std::cout << "error: failed to install keyboard hook. error code: " << error << std::endl;
        std::cout << "make sure to run as administrator." << std::endl;
        return 1;
    }
    
    std::cout << "keyboard hook installed successfully!" << std::endl;
    
    MSG msg;
    while (g_running) {
        BOOL bRet = GetMessage(&msg, NULL, 0, 0);
        if (bRet > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    if (g_hook) {
        UnhookWindowsHookEx(g_hook);
    }
    
    std::cout << "keyblocker stopped." << std::endl;
    return 0;
}

#define WIN32_LEAN_AND_MEAN
#define CORNER_SIZE 20
#include <stdlib.h>
#include <windows.h>

#pragma comment(lib, "USER32")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#define KEYDOWN(k) ((k) & 0x80)

// If mouse enters the rectangle, activate hotcorner function
static RECT kTopLeftHotCorner;
static RECT kTopRightHotCorner;

// Inputs to inject when corner activated
static const INPUT kVolumeUpInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_VOLUME_UP, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_VOLUME_UP, .dwFlags = KEYEVENTF_KEYUP}},
};

static const INPUT kVolumeDownInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_VOLUME_DOWN, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_VOLUME_DOWN, .dwFlags = KEYEVENTF_KEYUP}},
};

static const INPUT kDesktopLeftInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_CONTROL, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_LEFT, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_LEFT, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_CONTROL, .dwFlags = KEYEVENTF_KEYUP}},
};

static const INPUT kDesktopRightInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_CONTROL, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_RIGHT, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_RIGHT, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_CONTROL, .dwFlags = KEYEVENTF_KEYUP}},
};

static const INPUT kTaskViewInput[] = {
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_TAB, .dwFlags = 0}},
    {INPUT_KEYBOARD, .ki = {VK_TAB, .dwFlags = KEYEVENTF_KEYUP}},
    {INPUT_KEYBOARD, .ki = {VK_LWIN, .dwFlags = KEYEVENTF_KEYUP}},
};

// Update corner coordinates with the hotkey CTRL+ALT+F12
// Useful when resolution or DPI changes
// Quit application with ALT+SHIFT+F12
static const DWORD kHotKeyModUpdate = MOD_CONTROL | MOD_ALT;
static const DWORD kHotKeyModQuit = MOD_ALT | MOD_SHIFT;
static const DWORD kHotKey = VK_F12;

static BOOL NoModifierKeysPressedDown() {
    BYTE keyState[256];
    GetKeyState(0);
    if (GetKeyboardState(keyState)) {
        return !(KEYDOWN(keyState[VK_SHIFT]) || KEYDOWN(keyState[VK_CONTROL]) || KEYDOWN(keyState[VK_MENU]) ||
                 KEYDOWN(keyState[VK_LWIN]) || KEYDOWN(keyState[VK_LBUTTON]) || KEYDOWN(keyState[VK_RBUTTON]));
    }
    return 1;
}

static LRESULT HandleMouseWheelEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT *evt = (MSLLHOOKSTRUCT *)lParam;
    short wheelDelta = HIWORD(evt->mouseData);

    if (NoModifierKeysPressedDown()) {
        if (PtInRect(&kTopLeftHotCorner, evt->pt)) {
            if (wheelDelta > 0) {
                SendInput(_countof(kVolumeUpInput), kVolumeUpInput, sizeof(INPUT));
            } else {
                SendInput(_countof(kVolumeDownInput), kVolumeDownInput, sizeof(INPUT));
            }
            // Prevents the event from being handled by the application underneath
            return 1;
        }
        if (PtInRect(&kTopRightHotCorner, evt->pt)) {
            if (wheelDelta > 0) {
                SendInput(_countof(kDesktopLeftInput), kDesktopLeftInput, sizeof(INPUT));
            } else {
                SendInput(_countof(kDesktopRightInput), kDesktopRightInput, sizeof(INPUT));
            }
            return 1;
        }
    }

    // Pass the event to be handled by the next application in the chain
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static LRESULT HandleMiddleButtonUpEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT *evt = (MSLLHOOKSTRUCT *)lParam;

    if (PtInRect(&kTopLeftHotCorner, evt->pt) && NoModifierKeysPressedDown()) {
        SendInput(_countof(kTaskViewInput), kTaskViewInput, sizeof(INPUT));
        return 1;
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Mouse event handler
static LRESULT CALLBACK MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
    if (wParam == WM_MBUTTONDOWN) {
        return HandleMiddleButtonUpEvent(nCode, wParam, lParam);
    }
    if (wParam == WM_MOUSEWHEEL) {
        return HandleMouseWheelEvent(nCode, wParam, lParam);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Get coordinates for corners on the main screen
static void UpdateCorners() {
    int screenResX = GetSystemMetrics(SM_CXSCREEN);
    kTopLeftHotCorner.left = -CORNER_SIZE;
    kTopLeftHotCorner.top = -CORNER_SIZE;
    kTopLeftHotCorner.right = +CORNER_SIZE;
    kTopLeftHotCorner.bottom = +CORNER_SIZE;
    kTopRightHotCorner.left = screenResX - CORNER_SIZE;
    kTopRightHotCorner.top = -CORNER_SIZE;
    kTopRightHotCorner.right = screenResX + CORNER_SIZE;
    kTopRightHotCorner.bottom = +CORNER_SIZE;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MSG Msg;
    HHOOK MouseHook;

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetProcessDPIAware();
    UpdateCorners();

    if (!(MouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, NULL, 0))) {
        return 1;
    }

    RegisterHotKey(NULL, 1, kHotKeyModQuit, kHotKey);
    RegisterHotKey(NULL, 2, kHotKeyModUpdate, kHotKey);
    while (GetMessage(&Msg, NULL, 0, 0)) {
        if (Msg.message == WM_HOTKEY) {
            if (LOWORD(Msg.lParam) == kHotKeyModQuit) break;
            if (LOWORD(Msg.lParam) == kHotKeyModUpdate) UpdateCorners();
        }
        DispatchMessage(&Msg);
    }

    UnhookWindowsHookEx(MouseHook);
    return Msg.wParam;
}

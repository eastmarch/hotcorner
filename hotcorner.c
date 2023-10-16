#define WIN32_LEAN_AND_MEAN
#define CORNER_SIZE 20
#include <stdlib.h>
#include <windows.h>

#pragma comment(lib, "USER32")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// If mouse enters the rectangle, activate hotcorner function
static RECT kTopLeftHotCorner = {};
static RECT kTopRightHotCorner = {};

// Inputs to inject when corner activated
static const INPUT kVolumeUpInput[] = {
    { INPUT_KEYBOARD, .ki = { VK_VOLUME_UP, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_VOLUME_UP, .dwFlags = KEYEVENTF_KEYUP }},
};

static const INPUT kVolumeDownInput[] = {
    { INPUT_KEYBOARD, .ki = { VK_VOLUME_DOWN, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_VOLUME_DOWN, .dwFlags = KEYEVENTF_KEYUP }},
};

static const INPUT kDesktopLeftInput[] = {
    { INPUT_KEYBOARD, .ki = { VK_CONTROL, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_LEFT, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_LEFT, .dwFlags = KEYEVENTF_KEYUP }},
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = KEYEVENTF_KEYUP }},
    { INPUT_KEYBOARD, .ki = { VK_CONTROL, .dwFlags = KEYEVENTF_KEYUP }},
};

static const INPUT kDesktopRightInput[] = {
    { INPUT_KEYBOARD, .ki = { VK_CONTROL, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_RIGHT, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_RIGHT, .dwFlags = KEYEVENTF_KEYUP }},
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = KEYEVENTF_KEYUP }},
    { INPUT_KEYBOARD, .ki = { VK_CONTROL, .dwFlags = KEYEVENTF_KEYUP }},
};

static const INPUT kTaskViewInput[] = {
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_TAB,  .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_TAB,  .dwFlags = KEYEVENTF_KEYUP }},
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = KEYEVENTF_KEYUP }},
};

// Update corner coordinates with the hotkey CTRL+ALT+F12
// Useful when resolution or DPI changes
// Quit program with ALT+SHIFT+F12
static const DWORD kHotKeyModUpdate = MOD_CONTROL | MOD_ALT;
static const DWORD kHotKeyModQuit = MOD_ALT | MOD_SHIFT;
static const DWORD kHotKey = VK_F12;

static int modifierKeysPressedDown() {
    return GetAsyncKeyState(VK_SHIFT) 
        || GetAsyncKeyState(VK_CONTROL) 
        || GetAsyncKeyState(VK_MENU) 
        || GetAsyncKeyState(VK_LBUTTON) 
        || GetAsyncKeyState(VK_RBUTTON);
}

static LRESULT HandleMiddleButtonUpEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT *evt = (MSLLHOOKSTRUCT *) lParam;

    if (PtInRect(&kTopLeftHotCorner, evt->pt) && !modifierKeysPressedDown()) {
        SendInput(_countof(kTaskViewInput), kTaskViewInput, sizeof(INPUT));
    }
    // Pass the event to be handled by the next application in the chain
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static LRESULT HandleMouseWheelEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT *evt = (MSLLHOOKSTRUCT *) lParam;
    short wheelDelta = HIWORD(evt->mouseData);

	if (PtInRect(&kTopLeftHotCorner, evt->pt) && !modifierKeysPressedDown()) {
		if (wheelDelta > 0) {
			SendInput(_countof(kVolumeUpInput), kVolumeUpInput, sizeof(INPUT));
		} else {
			SendInput(_countof(kVolumeDownInput), kVolumeDownInput, sizeof(INPUT));
		}
		// This prevents an event from being handled by the application underneath
		return 1;
	}
	if (PtInRect(&kTopRightHotCorner, evt->pt) && !modifierKeysPressedDown()){
		if (wheelDelta > 0) {
			SendInput(_countof(kDesktopLeftInput), kDesktopLeftInput, sizeof(INPUT));
		} else {
			SendInput(_countof(kDesktopRightInput), kDesktopRightInput, sizeof(INPUT));
		}
		return 1;
	}
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Mouse event handler
static LRESULT CALLBACK MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
    if (wParam == WM_MBUTTONUP) {
        return HandleMiddleButtonUpEvent(nCode, wParam, lParam);
    }
    if (wParam == WM_MOUSEWHEEL) {
        return HandleMouseWheelEvent(nCode, wParam, lParam);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Get coordinates for corners other than top-left (main monitor)
static void UpdateCorners() {
    int screenResX = GetSystemMetrics(SM_CXSCREEN);
    kTopLeftHotCorner.left      =   -CORNER_SIZE;
	kTopLeftHotCorner.top       =   -CORNER_SIZE;
	kTopLeftHotCorner.right     =   +CORNER_SIZE;
    kTopLeftHotCorner.bottom    =   +CORNER_SIZE;
    kTopRightHotCorner.left     =   screenResX - CORNER_SIZE;
	kTopRightHotCorner.top      =   -CORNER_SIZE;
	kTopRightHotCorner.right    =   screenResX + CORNER_SIZE;
    kTopRightHotCorner.bottom   =   +CORNER_SIZE;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MSG Msg;
    HHOOK MouseHook;

	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetProcessDPIAware();
	UpdateCorners();
    
    if (!(MouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, NULL, 0))){
        return 1;
    }

    RegisterHotKey(NULL, 1, kHotKeyModQuit, kHotKey);
    RegisterHotKey(NULL, 2, kHotKeyModUpdate, kHotKey);
    while (GetMessage(&Msg, NULL, 0, 0)) {
        if (Msg.message == WM_HOTKEY) {
            if (LOWORD(Msg.lParam) == kHotKeyModQuit)
                break;
            if (LOWORD(Msg.lParam) == kHotKeyModUpdate)
                UpdateCorners();
        }
        DispatchMessage(&Msg);
    }

    UnhookWindowsHookEx(MouseHook);
    return Msg.wParam;
}

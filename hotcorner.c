#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <windows.h>

#pragma comment(lib, "USER32")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#define KEYDOWN(k) ((k) & 0x80)

// If mouse enters this rectangle, activate top-left hotcorner function.
static const RECT kTopLeftHotCorner = {
    .left   = -20,
	.top    = -20,
	.right  = +20,
    .bottom = +20,
};

// Screen resolution is needed for any other corner besides top-left.
static RECT kTopRightHotCorner = {};

// Inputs to inject when corner activated.
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

// Update corner coordinates with the hotkey CTRL+ALT+F12.
// Useful when resolution or DPI changes.
// Quit program with CTRL+ALT+SHIFT+F12.
static const DWORD kHotKeyModUpdate = MOD_CONTROL | MOD_ALT;
static const DWORD kHotKeyModQuit = MOD_CONTROL | MOD_ALT | MOD_SHIFT;
static const DWORD kHotKey = VK_F12;

// Mouse event handler
static LRESULT CALLBACK MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT *evt = (MSLLHOOKSTRUCT *) lParam;
    BYTE KeyState[256];

    // If there was no mouse wheel rotation, do nothing.
    if (wParam != WM_MOUSEWHEEL)
        goto finish;

    // Check if a mouse button is pressed
    if (GetKeyState(VK_LBUTTON) < 0 || GetKeyState(VK_RBUTTON) < 0) {
        goto finish;
    }

    // Check if any modifier keys are pressed.
    if (GetKeyboardState(KeyState)) {
        if (KEYDOWN(KeyState[VK_SHIFT]) || KEYDOWN(KeyState[VK_CONTROL])
          || KEYDOWN(KeyState[VK_MENU]) || KEYDOWN(KeyState[VK_LWIN])
          || KEYDOWN(KeyState[VK_RWIN])) {
            goto finish;
        }
    }

    // If hotcorner, we check for forwards or backwards mousewheel rotation.
    // And change the volume accordingly.
    short wheelDelta = HIWORD(evt->mouseData);
	if (PtInRect(&kTopLeftHotCorner, evt->pt)) {
		if (wheelDelta > 0) {
			SendInput(_countof(kVolumeUpInput), kVolumeUpInput, sizeof(INPUT));
		} else {
			SendInput(_countof(kVolumeDownInput), kVolumeDownInput, sizeof(INPUT));
		}
		//Prevents the mouse wheel event from being handled by the program underneath
		return 1;
	}
	
	//Switch virtual desktops with the mousewheel on the top-right hotcorner
	if (PtInRect(&kTopRightHotCorner, evt->pt)){
		if (wheelDelta > 0) {
			SendInput(_countof(kDesktopLeftInput), kDesktopLeftInput, sizeof(INPUT));
		} else {
			SendInput(_countof(kDesktopRightInput), kDesktopRightInput, sizeof(INPUT));
		}
		return 1;
	}
	

    finish:
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Get coordinates for corners other than top-left
void UpdateCorners(){
    int screenResX;
    screenResX = GetSystemMetrics(SM_CXSCREEN);
    kTopRightHotCorner.left     =   screenResX - 20;
	kTopRightHotCorner.top      =   -20;
	kTopRightHotCorner.right    =   screenResX + 20;
    kTopRightHotCorner.bottom   =   +20;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MSG Msg;
    HHOOK MouseHook;

	SetProcessDPIAware();
	UpdateCorners();
    
    if (!(MouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, NULL, 0))){
        return 1;
    }

    RegisterHotKey(NULL, 1, kHotKeyModQuit, kHotKey);
    RegisterHotKey(NULL, 2, kHotKeyModUpdate, kHotKey);
    while (GetMessage(&Msg, NULL, 0, 0)) {
        if (Msg.message == WM_HOTKEY) {
            switch (LOWORD(Msg.lParam)) {
                case kHotKeyModQuit: break;
                case kHotKeyModUpdate: UpdateCorners();
            }
        }
        DispatchMessage(&Msg);
    }

    UnhookWindowsHookEx(MouseHook);
    return Msg.wParam;
}

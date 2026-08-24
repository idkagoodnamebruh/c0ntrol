#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <string>

#include "src/platform/windows/WindowsSystemInputBackend.h"

namespace {

void printResult(const char* operation, bool success,
                 const std::string& detail = {}) {
    std::cout << operation << ": " << (success ? "PASS" : "FAIL");
    if (!detail.empty()) std::cout << " — " << detail;
    std::cout << '\n';
}

BOOL CALLBACK reportMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    auto* count = reinterpret_cast<int*>(data);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (GetMonitorInfoW(monitor, &info)) {
        ++*count;
        std::cout << "monitor[" << *count << "]: "
                  << info.rcMonitor.left << ',' << info.rcMonitor.top << " -> "
                  << info.rcMonitor.right << ',' << info.rcMonitor.bottom
                  << (info.dwFlags & MONITORINFOF_PRIMARY ? " PRIMARY" : "")
                  << '\n';
    }
    return TRUE;
}

struct CleanupGuard {
    WindowsSystemInputBackend* backend{nullptr};
    HWND window{nullptr};
    POINT original{};
    bool hasOriginal{false};
    bool buttonDown{false};
    bool active{true};

    ~CleanupGuard() {
        if (!active) return;
        if (buttonDown && backend != nullptr) {
            const bool released = backend->primaryButtonUp();
            printResult("cleanup primary UP", released,
                        released ? "" : backend->lastError());
        }
        if (hasOriginal) {
            printResult("cleanup cursor restore",
                        SetCursorPos(original.x, original.y) != FALSE);
        }
        if (backend != nullptr) backend->shutdown();
        if (window != nullptr) DestroyWindow(window);
    }
};

LRESULT CALLBACK smokeWindowProc(HWND window, UINT message, WPARAM wParam,
                                 LPARAM lParam) {
    if (message == WM_DESTROY) return 0;
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

int main() {
    std::cout << "c0ntrol native input smoke — MANUAL / OPT-IN\n";
    std::cout << "This tool never elevates privileges and is not a CTest.\n";

    WindowsSystemInputBackend backend;
    CleanupGuard cleanup;
    cleanup.backend = &backend;

    const bool initialized = backend.initialize();
    printResult("backend initialize", initialized,
                initialized ? "" : backend.lastError());
    if (!initialized) return 1;

    const DesktopGeometry desktop = backend.desktopGeometry();
    std::cout << "virtual desktop: origin=" << desktop.originX << ','
              << desktop.originY << " size=" << desktop.width << 'x'
              << desktop.height << '\n';
    int monitorCount = 0;
    EnumDisplayMonitors(nullptr, nullptr, reportMonitor,
                        reinterpret_cast<LPARAM>(&monitorCount));
    std::cout << "monitor count: " << monitorCount << '\n';

    const wchar_t* className = L"c0ntrolNativeInputSmokeWindow";
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = smokeWindowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground =
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&windowClass) && GetLastError() !=
                                             ERROR_CLASS_ALREADY_EXISTS) {
        printResult("test window create", false,
                    "RegisterClassW failed");
        return 1;
    }

    cleanup.window = CreateWindowExW(
        WS_EX_TOPMOST, className,
        L"c0ntrol native input smoke — controlled target",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        520, 360, nullptr, nullptr, windowClass.hInstance, nullptr);
    const bool windowCreated = cleanup.window != nullptr;
    printResult("test window create", windowCreated);
    if (!windowCreated) return 1;
    ShowWindow(cleanup.window, SW_SHOW);
    SetForegroundWindow(cleanup.window);
    UpdateWindow(cleanup.window);

    const int confirmation = MessageBoxW(
        cleanup.window,
        L"This opt-in test will move the pointer, press LEFT DOWN, make a "
        L"small drag entirely inside this window, release LEFT UP, and restore "
        L"the original cursor position. Continue?",
        L"Confirm native input smoke", MB_OKCANCEL | MB_ICONWARNING |
                                         MB_DEFBUTTON2);
    if (confirmation != IDOK) {
        std::cout << "operator confirmation: CANCELLED\n";
        return 2;
    }
    std::cout << "operator confirmation: YES\n";

    cleanup.hasOriginal = GetCursorPos(&cleanup.original) != FALSE;
    printResult("record original cursor", cleanup.hasOriginal);
    if (!cleanup.hasOriginal) return 1;

    RECT client{};
    if (!GetClientRect(cleanup.window, &client)) {
        printResult("client geometry", false);
        return 1;
    }
    POINT start{(client.right - client.left) / 2 - 30,
                (client.bottom - client.top) / 2};
    POINT dragged{start.x + 60, start.y + 20};
    if (!ClientToScreen(cleanup.window, &start) ||
        !ClientToScreen(cleanup.window, &dragged)) {
        printResult("client geometry", false, "ClientToScreen failed");
        return 1;
    }

    const bool moved = backend.movePointer({start.x, start.y});
    printResult("move inside test window", moved,
                moved ? "" : backend.lastError());
    if (!moved) return 1;
    Sleep(150);

    const bool down = backend.primaryButtonDown();
    cleanup.buttonDown = down;
    printResult("primary DOWN", down, down ? "" : backend.lastError());
    if (!down) return 1;

    const bool dragMoved = backend.movePointer({dragged.x, dragged.y});
    printResult("move-drag inside test window", dragMoved,
                dragMoved ? "" : backend.lastError());
    if (!dragMoved) return 1;

    const bool up = backend.primaryButtonUp();
    if (up) cleanup.buttonDown = false;
    printResult("primary UP", up, up ? "" : backend.lastError());
    if (!up) return 1;

    const bool restored =
        backend.movePointer({cleanup.original.x, cleanup.original.y});
    printResult("restore cursor", restored,
                restored ? "" : backend.lastError());
    if (!restored) return 1;

    backend.shutdown();
    cleanup.backend = nullptr;
    DestroyWindow(cleanup.window);
    cleanup.window = nullptr;
    cleanup.active = false;
    std::cout << "native smoke result: PASS\n";
    return 0;
}

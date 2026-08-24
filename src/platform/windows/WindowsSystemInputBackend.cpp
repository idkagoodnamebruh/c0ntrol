#include "WindowsSystemInputBackend.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sstream>
#include <cstdint>
#include <limits>

#include "src/platform/windows/WindowsPointerMath.h"

bool WindowsSystemInputBackend::initialize() {
    m_desktop = {
        GetSystemMetrics(SM_XVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_CYVIRTUALSCREEN),
    };
    if (!m_desktop.isValid()) {
        m_lastError = "GetSystemMetrics returned invalid virtual desktop geometry";
        return false;
    }
    m_initialized = true;
    m_lastError.clear();
    return true;
}

bool WindowsSystemInputBackend::sendMouse(unsigned long flags, long x, long y,
                                          long mouseData) {
    if (!m_initialized) {
        m_lastError = "Windows input backend is not initialized";
        return false;
    }

    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = x;
    input.mi.dy = y;
    input.mi.mouseData = static_cast<DWORD>(mouseData);
    input.mi.dwFlags = flags;
    SetLastError(ERROR_SUCCESS);
    const UINT sent = SendInput(1, &input, sizeof(INPUT));
    if (sent != 1) {
        const DWORD error = GetLastError();
        std::ostringstream stream;
        stream << "SendInput inserted " << sent << "/1 events; GetLastError="
               << error << " (UIPI blocking is not distinguishable)";
        m_lastError = stream.str();
        return false;
    }
    m_lastError.clear();
    return true;
}

bool WindowsSystemInputBackend::movePointer(const DesktopPoint& point) {
    const auto absolute = desktopPixelToSendInputAbsolute(point, m_desktop);
    if (!absolute.has_value()) {
        m_lastError = "cannot map pointer against invalid virtual desktop";
        return false;
    }
    return sendMouse(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE |
                         MOUSEEVENTF_VIRTUALDESK,
                     absolute->x, absolute->y);
}

bool WindowsSystemInputBackend::primaryButtonDown() {
    return sendMouse(MOUSEEVENTF_LEFTDOWN);
}

bool WindowsSystemInputBackend::primaryButtonUp() {
    return sendMouse(MOUSEEVENTF_LEFTUP);
}

bool WindowsSystemInputBackend::scrollVertical(int notches) {
    if (notches == 0) {
        m_lastError = "vertical scroll amount must be non-zero";
        return false;
    }
    const std::int64_t wheelDelta =
        static_cast<std::int64_t>(notches) * WHEEL_DELTA;
    if (wheelDelta < std::numeric_limits<long>::min() ||
        wheelDelta > std::numeric_limits<long>::max()) {
        m_lastError = "vertical scroll amount exceeds Windows wheel range";
        return false;
    }
    return sendMouse(MOUSEEVENTF_WHEEL, 0, 0,
                     static_cast<long>(wheelDelta));
}

void WindowsSystemInputBackend::shutdown() {
    m_initialized = false;
}

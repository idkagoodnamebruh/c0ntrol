#include "CursorController.h"

CursorController::CursorController(QObject* parent)
    : QObject(parent), m_enabled(true), m_lastX(0), m_lastY(0) {
    
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        m_screenGeometry = screen->geometry();
    } else {
        m_screenGeometry = QRect(0, 0, 1920, 1080);
    }
}

void CursorController::onPointerUpdated(const Point3D& pointer, bool active) {
    if (!m_enabled || !active) return;

    int targetX = static_cast<int>(pointer.x * m_screenGeometry.width());
    int targetY = static_cast<int>(pointer.y * m_screenGeometry.height());
    targetX = std::clamp(targetX, 0, m_screenGeometry.width() - 1);
    targetY = std::clamp(targetY, 0, m_screenGeometry.height() - 1);
    moveCursor(targetX, targetY);
}

void CursorController::moveCursor(int x, int y) {
    m_lastX = x;
    m_lastY = y;

    QCursor::setPos(x, y);
    emit cursorMoved(x, y);
}

void CursorController::performClick(Qt::MouseButton button) {
    emit clickPerformed(button);
}

void CursorController::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool CursorController::isEnabled() const {
    return m_enabled;
}

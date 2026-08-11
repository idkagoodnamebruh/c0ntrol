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

void CursorController::onLandmarksUpdated(const Landmarks& landmarks, GestureType gesture) {
    if (!m_enabled || landmarks.points.empty()) return;

    // Usar la punta del dedo índice (Landmark 8) para la posición del cursor
    if (landmarks.points.size() > 8) {
        Point3D indexTip = landmarks.points[8];
        
        // Mapear coordenadas normalizadas (0.0 - 1.0) a la resolución de la pantalla
        int targetX = static_cast<int>(indexTip.x * m_screenGeometry.width());
        int targetY = static_cast<int>(indexTip.y * m_screenGeometry.height());

        // Limitar dentro de los bordes de la pantalla
        targetX = std::clamp(targetX, 0, m_screenGeometry.width() - 1);
        targetY = std::clamp(targetY, 0, m_screenGeometry.height() - 1);

        if (gesture == GestureType::POINTING || gesture == GestureType::PINCH) {
            moveCursor(targetX, targetY);
        }

        if (gesture == GestureType::PINCH) {
            performClick(Qt::LeftButton);
        }
    }
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

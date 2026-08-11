#include "MinimalistModeWidget.h"
#include <QPainter>

MinimalistModeWidget::MinimalistModeWidget(QWidget* parent)
    : QWidget(parent), m_gestureName("NONE") {
    
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::SubWindow);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(200, 60);
}

void MinimalistModeWidget::updateHUD(const QString& gestureName) {
    m_gestureName = gestureName;
    update();
}

void MinimalistModeWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fondo semi-transparente estilo HUD Glassmorphic
    QColor bgColor(20, 20, 20, 180);
    painter.setBrush(bgColor);
    painter.setPen(QPen(QColor(0, 255, 150), 2));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);

    // Texto de gesto activo
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, QString("c0ntrol: %1").arg(m_gestureName));
}

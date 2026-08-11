#include "MatrixRainWidget.h"
#include <QRandomGenerator>

MatrixRainWidget::MatrixRainWidget(QWidget* parent)
    : QWidget(parent) {

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MatrixRainWidget::onTimerTimeout);
    m_timer->start(50); // 20 FPS animación rain
}

void MatrixRainWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    painter.setPen(Qt::green);
    painter.setFont(QFont("Monospace", 12, QFont::Bold));

    for (const auto& drop : m_drops) {
        painter.drawText(drop.x, drop.y, QString(drop.character));
    }
}

void MatrixRainWidget::onTimerTimeout() {
    if (m_drops.size() < 50) {
        RainDrop drop;
        drop.x = QRandomGenerator::global()->bounded(width() > 0 ? width() : 800);
        drop.y = 0;
        drop.speed = QRandomGenerator::global()->bounded(5, 15);
        drop.character = static_cast<char>(QRandomGenerator::global()->bounded(33, 126));
        m_drops.push_back(drop);
    }

    for (auto& drop : m_drops) {
        drop.y += drop.speed;
        if (drop.y > height()) {
            drop.y = 0;
            drop.x = QRandomGenerator::global()->bounded(width() > 0 ? width() : 800);
        }
    }

    update();
}

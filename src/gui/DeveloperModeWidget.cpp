#include "DeveloperModeWidget.h"

DeveloperModeWidget::DeveloperModeWidget(QWidget* parent)
    : QWidget(parent), m_fps(30.0), m_gestureName("NONE") {
    
    m_mainLayout = new QVBoxLayout(this);
    
    m_fpsLabel = new QLabel("FPS: --", this);
    m_gestureLabel = new QLabel("Gesto: NONE", this);
    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setMaximumHeight(150);

    m_mainLayout->addWidget(m_fpsLabel);
    m_mainLayout->addWidget(m_gestureLabel);
    m_mainLayout->addWidget(m_logText);

    setLayout(m_mainLayout);
}

void DeveloperModeWidget::updateTelemetry(double fps, const QString& gestureName, const Landmarks& landmarks) {
    m_fps = fps;
    m_gestureName = gestureName;

    m_fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
    m_gestureLabel->setText(QString("Gesto Detectado: %1").arg(gestureName));

    if (!landmarks.points.empty()) {
        QString log = QString("[%1] Wrist X: %2 Y: %3")
                          .arg(gestureName)
                          .arg(landmarks.points[0].x, 0, 'f', 2)
                          .arg(landmarks.points[0].y, 0, 'f', 2);
        m_logText->append(log);
    }
}

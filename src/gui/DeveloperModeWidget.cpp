#include "DeveloperModeWidget.h"

DeveloperModeWidget::DeveloperModeWidget(QWidget* parent)
    : QWidget(parent), m_gestureName("NONE") {
    
    m_mainLayout = new QVBoxLayout(this);
    
    m_captureFpsLabel = new QLabel("Capture FPS: --", this);
    m_processingFpsLabel = new QLabel("Processing FPS: --", this);
    m_dropLabel = new QLabel("Overwritten: 0", this);
    m_latencyLabel = new QLabel("Frame age: -- ms | Inference: -- ms", this);
    m_gestureLabel = new QLabel("Gesto: NONE", this);

    m_mainLayout->addWidget(m_captureFpsLabel);
    m_mainLayout->addWidget(m_processingFpsLabel);
    m_mainLayout->addWidget(m_dropLabel);
    m_mainLayout->addWidget(m_latencyLabel);
    m_mainLayout->addWidget(m_gestureLabel);

    setLayout(m_mainLayout);
}

void DeveloperModeWidget::updateTelemetry(const PipelineMetrics& metrics,
                                          const QString& gestureName) {
    m_gestureName = gestureName;

    m_captureFpsLabel->setText(
        QString("Capture FPS: %1").arg(metrics.captureFps, 0, 'f', 1));
    m_processingFpsLabel->setText(
        QString("Processing FPS: %1").arg(metrics.processingFps, 0, 'f', 1));
    m_dropLabel->setText(
        QString("Captured: %1 | Processed: %2 | Overwritten: %3")
            .arg(metrics.capturedFrames)
            .arg(metrics.processedFrames)
            .arg(metrics.overwrittenFrames));
    m_latencyLabel->setText(
        QString("Frame age: %1 ms | Inference: %2 ms | Processing: %3 ms")
            .arg(metrics.frameAgeAtProcessingUs / 1000.0, 0, 'f', 1)
            .arg(metrics.inferenceDurationUs / 1000.0, 0, 'f', 1)
            .arg(metrics.processingDurationUs / 1000.0, 0, 'f', 1));
    m_gestureLabel->setText(QString("Gesto Detectado: %1").arg(gestureName));
}

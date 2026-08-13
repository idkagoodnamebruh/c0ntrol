#include "VisionWorker.h"
#include <QThread>
#include <QDebug>
#include <cmath>

VisionWorker::VisionWorker(QObject* parent)
    : QObject(parent), m_cameraIndex(0), m_frameTimer(new QTimer(this)), m_mockTime(0.0) {
    m_filter = std::make_unique<OneEuroFilter>();
    m_frameTimer->setInterval(33);
    connect(m_frameTimer, &QTimer::timeout, this, &VisionWorker::processFrame);
}

VisionWorker::~VisionWorker() {
    stop();
}

void VisionWorker::setCameraIndex(int index) {
    m_cameraIndex = index;
}

void VisionWorker::start() {
    if (m_frameTimer->isActive()) {
        return;
    }

    m_cap.open(m_cameraIndex);

    if (!m_cap.isOpened()) {
        qWarning() << "[ERROR] No se pudo abrir la cámara index:" << m_cameraIndex;
        emit errorOccurred("No se pudo acceder a la cámara seleccionada.");
        return;
    }

    m_cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    qInfo() << "[INFO] Hilo de capturas iniciado exitosamente.";
    m_frameTimer->start();
}

void VisionWorker::stop() {
    m_frameTimer->stop();
    if (m_cap.isOpened()) {
        m_cap.release();
    }
}

void VisionWorker::processFrame() {
    if (!m_cap.isOpened()) {
        return;
    }

    cv::Mat frame;
    m_cap >> frame;
    if (frame.empty()) {
        return;
    }

        // Convertir BGR OpenCV a QImage RGB888 para Qt QPainter
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

        QImage image(rgbFrame.data, rgbFrame.cols, rgbFrame.rows, rgbFrame.step, QImage::Format_RGB888);

        // Generar landmarks suavizados con OneEuroFilter
    Landmarks landmarks = extractLandmarksMock(m_mockTime);
    m_mockTime += 0.033;

    emit frameProcessed(image.copy(), landmarks);
}

Landmarks VisionWorker::extractLandmarksMock(double t) {
    Landmarks lm;
    lm.points.resize(21);

    // Movimiento fluido del punto wrist y punta del índice
    double baseX = 0.5 + 0.2 * std::sin(t);
    double baseY = 0.5 + 0.2 * std::cos(t);

    for (int i = 0; i < 21; ++i) {
        double offsetX = (i * 0.01) * std::sin(t * 2.0);
        double offsetY = (i * 0.01) * std::cos(t * 2.0);

        Point3D rawPoint(baseX + offsetX, baseY + offsetY, 0.0);
        lm.points[i] = m_filter->filterPoint(rawPoint, t);
    }

    return lm;
}

#include "VisionWorker.h"
#include <QThread>
#include <QDebug>
#include <cmath>
#include "src/core/tracking/LegacyLandmarksAdapter.h"
#include "src/core/tracking/MockHandTrackingBackend.h"
#ifdef C0NTROL_ENABLE_MEDIAPIPE
#include "src/core/tracking/MediaPipeHandTrackingBackend.h"
#endif

VisionWorker::VisionWorker(QObject* parent)
    : QObject(parent), m_cameraIndex(0), m_frameTimer(new QTimer(this)) {
#ifdef C0NTROL_ENABLE_MEDIAPIPE
    m_trackingBackend = std::make_unique<MediaPipeHandTrackingBackend>();
#else
    m_trackingBackend = std::make_unique<MockHandTrackingBackend>();
#endif
    m_frameTimer->setInterval(33);
    connect(m_frameTimer, &QTimer::timeout, this, &VisionWorker::processFrame);
}

VisionWorker::~VisionWorker() {
    stop();
}

void VisionWorker::setCameraIndex(int index) {
    m_cameraIndex = index;
}

void VisionWorker::setFilteringEnabled(bool enabled) {
    m_landmarkFilterBank.setEnabled(enabled);
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

    HandTrackingConfig trackingConfig;
    if (!m_trackingBackend->initialize(trackingConfig)) {
        emit errorOccurred(QString::fromStdString(m_trackingBackend->lastError()));
        m_cap.release();
        return;
    }

    m_landmarkFilterBank.reset();
    qInfo() << "[INFO] Hilo de capturas iniciado exitosamente.";
    m_frameTimer->start();
}

void VisionWorker::stop() {
    m_frameTimer->stop();
    m_trackingBackend->shutdown();
    m_landmarkFilterBank.reset();
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

    const auto timestampUs = m_trackingClock.nextTimestampUs();
    const auto frameId = m_trackingClock.nextFrameId();
    const RgbImageView imageView{rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
                                 static_cast<std::size_t>(rgbFrame.step)};
    HandTrackingFrame rawTrackingFrame =
        m_trackingBackend->process(imageView, timestampUs, frameId);
    if (!rawTrackingFrame.valid && !m_trackingBackend->lastError().empty()) {
        emit errorOccurred(QString::fromStdString(m_trackingBackend->lastError()));
    }

    // Keep the backend observation intact for debugging and future consumers.
    emit trackingFrameProcessed(rawTrackingFrame);

    HandTrackingFrame filteredTrackingFrame =
        m_landmarkFilterBank.process(rawTrackingFrame);
    emit filteredTrackingFrameProcessed(filteredTrackingFrame);

    // Temporary compatibility policy: prefer a RIGHT hand, otherwise the first.
    // The legacy gesture/GUI path consumes the stabilized observation.
    Landmarks landmarks = toLegacyLandmarks(filteredTrackingFrame);
    emit frameProcessed(image.copy(), landmarks);
}

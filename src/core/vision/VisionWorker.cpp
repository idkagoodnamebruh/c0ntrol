#include "VisionWorker.h"
#include <QDebug>
#include <chrono>
#include <cmath>
#include "src/core/tracking/LegacyLandmarksAdapter.h"
#include "src/core/tracking/MockHandTrackingBackend.h"
#ifdef C0NTROL_ENABLE_MEDIAPIPE
#include "src/core/tracking/MediaPipeHandTrackingBackend.h"
#endif

VisionWorker::VisionWorker(CameraConfig cameraConfig,
                           LandmarkFilterConfig filterConfig,
                           QObject* parent)
    : QObject(parent),
      m_cameraConfig(sanitizeCameraConfig(cameraConfig)),
      m_consumerTimer(new QTimer(this)),
      m_landmarkFilterBank(sanitizeLandmarkFilterConfig(filterConfig)) {
#ifdef C0NTROL_ENABLE_MEDIAPIPE
    m_trackingBackend = std::make_unique<MediaPipeHandTrackingBackend>();
#else
    m_trackingBackend = std::make_unique<MockHandTrackingBackend>();
#endif
    auto source = std::make_unique<OpenCVCameraSource>(m_cameraConfig);
    m_captureSource = source.get();
    m_capture = std::make_unique<AsyncCapture<cv::Mat>>(std::move(source));

    // The timer only checks a capacity-one slot; it never reads the camera.
    // Five milliseconds is bounded polling, not an assumed camera FPS.
    m_consumerTimer->setTimerType(Qt::PreciseTimer);
    m_consumerTimer->setInterval(5);
    connect(m_consumerTimer, &QTimer::timeout,
            this, &VisionWorker::processLatestFrame);
}

VisionWorker::~VisionWorker() {
    stop();
}

void VisionWorker::setFilteringEnabled(bool enabled) {
    m_landmarkFilterBank.setEnabled(enabled);
}

void VisionWorker::applyConfiguration(
    const CameraConfig& cameraConfig,
    const LandmarkFilterConfig& filterConfig) {
    const CameraConfig sanitizedCamera = sanitizeCameraConfig(cameraConfig);
    const LandmarkFilterConfig sanitizedFilter =
        sanitizeLandmarkFilterConfig(filterConfig);
    const bool cameraChanged = sanitizedCamera != m_cameraConfig;
    const bool wasRunning = m_running;
    if (cameraChanged && wasRunning) stop();

    m_cameraConfig = sanitizedCamera;
    if (cameraChanged && m_captureSource != nullptr)
        (void)m_captureSource->setConfig(m_cameraConfig);
    if (sanitizedFilter != m_landmarkFilterBank.config())
        m_landmarkFilterBank = LandmarkFilterBank(sanitizedFilter);

    if (cameraChanged && wasRunning) start();
}

void VisionWorker::start() {
    if (m_running) return;

    HandTrackingConfig trackingConfig;
    if (!m_trackingBackend->initialize(trackingConfig)) {
        emit errorOccurred(
            "TRACKING_FAILED: " +
            QString::fromStdString(m_trackingBackend->lastError()));
        return;
    }

    m_landmarkFilterBank.reset();
    m_metricsTracker.reset();
    m_lastTrackingTimestampUs = -1;
    m_lastTelemetryEmitUs = -1;
    m_lastErrorEmitUs = -1;
    m_lastErrorMessage.clear();
    m_captureStartedReported = false;
    m_captureFailureReported = false;
    m_running = m_capture->start();
    if (!m_running) {
        m_trackingBackend->shutdown();
        emit errorOccurred("CAMERA_OPEN_FAILED: capture producer did not start");
        return;
    }
    m_consumerTimer->start();
}

void VisionWorker::stop() {
    const bool wasActive = m_running ||
        (m_capture && m_capture->state() != CaptureState::STOPPED);
    m_consumerTimer->stop();
    m_running = false;
    if (m_capture) m_capture->stop();
    m_trackingBackend->shutdown();
    m_landmarkFilterBank.reset();
    if (wasActive) qInfo() << "[CAPTURE] stopped";
}

std::int64_t VisionWorker::steadyNowUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void VisionWorker::reportCaptureStarted() {
    if (m_captureStartedReported || m_captureSource == nullptr) return;
    const CameraInfo info = m_captureSource->cameraInfo();
    const char* bufferSupport = "UNKNOWN";
    if (info.bufferSizeSupport == CameraPropertySupport::SUPPORTED)
        bufferSupport = "SUPPORTED";
    else if (info.bufferSizeSupport == CameraPropertySupport::NOT_SUPPORTED)
        bufferSupport = "NOT_SUPPORTED";

    qInfo() << "[CAPTURE] started"
            << "backend=" << QString::fromStdString(info.backendName)
            << "requested=" << info.requested.requestedWidth << "x"
            << info.requested.requestedHeight << "@"
            << info.requested.requestedFps
            << "actual=" << info.actualWidth << "x" << info.actualHeight
            << "@" << info.actualFps
            << "buffer-size=" << bufferSupport;
    m_captureStartedReported = true;
}

void VisionWorker::handleCaptureFailure() {
    if (m_captureFailureReported) return;
    m_captureFailureReported = true;
    const QString detail = QString::fromStdString(m_capture->lastError());
    emit errorOccurred(detail.isEmpty()
        ? QString("CAMERA_DISCONNECTED: capture producer failed")
        : detail);
    m_consumerTimer->stop();
    m_running = false;
    m_capture->stop();
    m_trackingBackend->shutdown();
}

void VisionWorker::emitRateLimitedError(const QString& category,
                                        const QString& detail) {
    const auto nowUs = steadyNowUs();
    const QString message = category + ": " + detail;
    if (message != m_lastErrorMessage || m_lastErrorEmitUs < 0 ||
        nowUs - m_lastErrorEmitUs >= 1'000'000) {
        emit errorOccurred(message);
        m_lastErrorMessage = message;
        m_lastErrorEmitUs = nowUs;
    }
}

void VisionWorker::processLatestFrame() {
    if (!m_running) return;

    const CaptureState state = m_capture->state();
    if (state == CaptureState::FAILED) {
        handleCaptureFailure();
        return;
    }
    if (state == CaptureState::STOPPED && m_captureStartedReported) {
        if (!m_captureFailureReported) {
            m_captureFailureReported = true;
            emit errorOccurred(
                "CAMERA_DISCONNECTED: capture producer stopped unexpectedly");
        }
        m_consumerTimer->stop();
        m_running = false;
        m_capture->stop();
        m_trackingBackend->shutdown();
        return;
    }
    if (state == CaptureState::RUNNING) reportCaptureStarted();

    auto captured = m_capture->tryTakeLatest();
    if (!captured.has_value()) return;

    const auto processingStartUs = steadyNowUs();
    cv::Mat rgbFrame;
    try {
        cv::cvtColor(captured->value, rgbFrame, cv::COLOR_BGR2RGB);
    } catch (const cv::Exception& exception) {
        emitRateLimitedError("TRACKING_FAILED", exception.what());
        return;
    }

    auto trackingTimestampUs = captured->metadata.captureTimestampUs;
    if (trackingTimestampUs <= m_lastTrackingTimestampUs)
        trackingTimestampUs = m_lastTrackingTimestampUs + 1;
    m_lastTrackingTimestampUs = trackingTimestampUs;

    const auto inferenceStartUs = steadyNowUs();
    const RgbImageView imageView{rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
                                 static_cast<std::size_t>(rgbFrame.step)};
    HandTrackingFrame rawTrackingFrame = m_trackingBackend->process(
        imageView, trackingTimestampUs,
        captured->metadata.captureSequence);
    const auto inferenceEndUs = steadyNowUs();
    if (!rawTrackingFrame.valid && !m_trackingBackend->lastError().empty()) {
        emitRateLimitedError(
            "TRACKING_FAILED",
            QString::fromStdString(m_trackingBackend->lastError()));
    }

    emit trackingFrameProcessed(rawTrackingFrame);

    HandTrackingFrame filteredTrackingFrame =
        m_landmarkFilterBank.process(rawTrackingFrame);
    emit filteredTrackingFrameProcessed(filteredTrackingFrame);

    Landmarks landmarks = toLegacyLandmarks(filteredTrackingFrame);
    QImage image(rgbFrame.data, rgbFrame.cols, rgbFrame.rows, rgbFrame.step,
                 QImage::Format_RGB888);
    QImage ownedImage = image.copy();
    const auto processingEndUs = steadyNowUs();
    m_metricsTracker.recordProcessed(
        captured->metadata.captureTimestampUs, processingStartUs,
        inferenceStartUs, inferenceEndUs, processingEndUs);

    emit frameProcessed(ownedImage, landmarks);

    if (m_lastTelemetryEmitUs < 0 ||
        processingEndUs - m_lastTelemetryEmitUs >= 200'000) {
        emit metricsUpdated(m_metricsTracker.snapshot(m_capture->metrics()));
        m_lastTelemetryEmitUs = processingEndUs;
    }
}

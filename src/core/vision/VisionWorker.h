#ifndef VISIONWORKER_H
#define VISIONWORKER_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <cstdint>
#include <memory>
#include <opencv2/opencv.hpp>

#include "src/core/capture/AsyncCapture.h"
#include "src/core/gestures/Landmarks.h"
#include "src/core/filters/LandmarkFilterBank.h"
#include "src/core/metrics/PipelineMetrics.h"
#include "src/core/qt/QtMetaTypes.h"
#include "src/core/tracking/IHandTrackingBackend.h"
#include "src/platform/opencv/OpenCVCameraSource.h"

class VisionWorker : public QObject {
    Q_OBJECT

public:
    explicit VisionWorker(QObject* parent = nullptr);
    ~VisionWorker();

    void setCameraIndex(int index);
    bool filteringEnabled() const { return m_landmarkFilterBank.enabled(); }

public slots:
    void start();
    void stop();
    void setFilteringEnabled(bool enabled);

signals:
    void frameProcessed(const QImage& frame, const Landmarks& landmarks);
    void trackingFrameProcessed(const HandTrackingFrame& trackingFrame);
    void filteredTrackingFrameProcessed(const HandTrackingFrame& trackingFrame);
    void metricsUpdated(const PipelineMetrics& metrics);
    void errorOccurred(const QString& errorMessage);

private:
    void processLatestFrame();
    void reportCaptureStarted();
    void handleCaptureFailure();
    void emitRateLimitedError(const QString& category,
                              const QString& detail);
    static std::int64_t steadyNowUs();

    int m_cameraIndex;
    CameraConfig m_cameraConfig;
    OpenCVCameraSource* m_captureSource{nullptr};
    std::unique_ptr<AsyncCapture<cv::Mat>> m_capture;
    std::unique_ptr<IHandTrackingBackend> m_trackingBackend;
    QTimer* m_consumerTimer;
    LandmarkFilterBank m_landmarkFilterBank;
    PipelineMetricsTracker m_metricsTracker;
    bool m_running{false};
    bool m_captureStartedReported{false};
    bool m_captureFailureReported{false};
    std::int64_t m_lastTrackingTimestampUs{-1};
    std::int64_t m_lastTelemetryEmitUs{-1};
    std::int64_t m_lastErrorEmitUs{-1};
    QString m_lastErrorMessage;
};

#endif // VISIONWORKER_H

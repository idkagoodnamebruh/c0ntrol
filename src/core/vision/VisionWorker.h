#ifndef VISIONWORKER_H
#define VISIONWORKER_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <memory>
#include <opencv2/opencv.hpp>

#include "src/core/gestures/Landmarks.h"
#include "src/core/filters/OneEuroFilter.h"
#include "src/core/qt/QtMetaTypes.h"
#include "src/core/tracking/IHandTrackingBackend.h"
#include "src/core/tracking/TrackingClock.h"

class VisionWorker : public QObject {
    Q_OBJECT

public:
    explicit VisionWorker(QObject* parent = nullptr);
    ~VisionWorker();

    void setCameraIndex(int index);

public slots:
    void start();
    void stop();

signals:
    void frameProcessed(const QImage& frame, const Landmarks& landmarks);
    void trackingFrameProcessed(const HandTrackingFrame& trackingFrame);
    void errorOccurred(const QString& errorMessage);

private:
    void processFrame();
    int m_cameraIndex;
    cv::VideoCapture m_cap;
    std::unique_ptr<IHandTrackingBackend> m_trackingBackend;
    QTimer* m_frameTimer;
    TrackingClock m_trackingClock;
};

#endif // VISIONWORKER_H

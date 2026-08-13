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
    void errorOccurred(const QString& errorMessage);

private:
    void processFrame();
    Landmarks extractLandmarksMock(double t);

    int m_cameraIndex;
    cv::VideoCapture m_cap;
    std::unique_ptr<OneEuroFilter> m_filter;
    QTimer* m_frameTimer;
    double m_mockTime;
};

#endif // VISIONWORKER_H
